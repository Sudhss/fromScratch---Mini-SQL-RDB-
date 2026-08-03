#include "engine/executor.h"
#include "storage/table.h"
#include "storage/btree.h"
#include "core/errors.h"
#include <algorithm>
#include <map>

namespace minidb {

Executor::Executor(Catalog& catalog, Pager& pager) : catalog(catalog), pager(pager) {}

QueryResult Executor::execute(const Statement& stmt) {
    return std::visit([&](auto&& arg) -> QueryResult {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, SelectStmt>) return executeSelect(arg);
        else if constexpr (std::is_same_v<T, InsertStmt>) return executeInsert(arg);
        else if constexpr (std::is_same_v<T, UpdateStmt>) return executeUpdate(arg);
        else if constexpr (std::is_same_v<T, DeleteStmt>) return executeDelete(arg);
        else if constexpr (std::is_same_v<T, CreateTableStmt>) return executeCreateTable(arg);
        else if constexpr (std::is_same_v<T, DropTableStmt>) return executeDropTable(arg);
        else if constexpr (std::is_same_v<T, ShowTablesStmt>) return executeShowTables(arg);
        else if constexpr (std::is_same_v<T, DescribeStmt>) return executeDescribe(arg);
        else throw Exception("Unsupported statement type");
    }, stmt);
}

Value Executor::accumulateAggregate(const FunctionCallExpr& func, const QVector<Row>& groupRows, const EvalContext& context) {
    QString fnName = func.functionName.toUpper();
    if (fnName == "COUNT") {
        if (func.arguments.empty()) {
            return Value(static_cast<int>(groupRows.size()));
        }
        int count = 0;
        for (const auto& row : groupRows) {
            Value val = Evaluator::evaluate(func.arguments[0], row, context);
            if (!std::holds_alternative<std::monostate>(val)) {
                count++;
            }
        }
        return Value(count);
    } else if (fnName == "SUM") {
        int sum = 0;
        bool hasNonNull = false;
        for (const auto& row : groupRows) {
            Value val = Evaluator::evaluate(func.arguments[0], row, context);
            if (std::holds_alternative<int>(val)) {
                sum += std::get<int>(val);
                hasNonNull = true;
            }
        }
        return hasNonNull ? Value(sum) : Value(std::monostate{});
    } else if (fnName == "AVG") {
        int sum = 0;
        int count = 0;
        for (const auto& row : groupRows) {
            Value val = Evaluator::evaluate(func.arguments[0], row, context);
            if (std::holds_alternative<int>(val)) {
                sum += std::get<int>(val);
                count++;
            }
        }
        return count > 0 ? Value(sum / count) : Value(std::monostate{});
    } else if (fnName == "MIN") {
        std::optional<Value> minVal;
        for (const auto& row : groupRows) {
            Value val = Evaluator::evaluate(func.arguments[0], row, context);
            if (!std::holds_alternative<std::monostate>(val)) {
                if (!minVal.has_value() || val < minVal.value()) {
                    minVal = val;
                }
            }
        }
        return minVal.has_value() ? minVal.value() : Value(std::monostate{});
    } else if (fnName == "MAX") {
        std::optional<Value> maxVal;
        for (const auto& row : groupRows) {
            Value val = Evaluator::evaluate(func.arguments[0], row, context);
            if (!std::holds_alternative<std::monostate>(val)) {
                if (!maxVal.has_value() || val > maxVal.value()) {
                    maxVal = val;
                }
            }
        }
        return maxVal.has_value() ? maxVal.value() : Value(std::monostate{});
    }
    throw Exception("Unsupported aggregate function: " + fnName);
}

QueryResult Executor::executeSelect(const SelectStmt& stmt) {
    if (stmt.fromTables.empty()) {
        QueryResult result;
        Row row;
        for (const auto& expr : stmt.columns) {
            TableSchema emptySchema;
            emptySchema.tableName = "";
            row.append(Evaluator::evaluate(expr, Row(), emptySchema));
            result.columns.append("expr");
        }
        result.rows.append(row);
        return result;
    }

    Planner planner(catalog);
    QueryPlan plan = planner.plan(stmt);

    QVector<QPair<QString, TableSchema>> schemas;
    QVector<Table> tables;
    QVector<BTree> btrees;

    for (const auto& tRef : stmt.fromTables) {
        if (!catalog.tableExists(tRef.tableName)) {
            throw Exception("Table does not exist: " + tRef.tableName);
        }
        TableSchema schema = catalog.getTableSchema(tRef.tableName);
        QString alias = tRef.alias.has_value() ? tRef.alias.value() : tRef.tableName;
        schemas.append({alias, schema});
        
        tables.emplace_back(pager, schema.rootPage);
        btrees.emplace_back(pager, schema.rootPage);
    }

    EvalContext context(schemas);
    QVector<Row> joinedRows;

    // Scan the first table
    if (plan.useIndex && plan.scanType == ScanType::INDEX_LOOKUP) {
        Value val = btrees[0].find(plan.indexKey);
        if (!std::holds_alternative<std::monostate>(val)) {
            joinedRows.append(tables[0].readRow(std::get<int>(val)));
        }
    } else {
        auto allRows = tables[0].fullScan();
        for (const auto& pr : allRows) joinedRows.append(pr.second);
    }

    // Process JOINs
    for (int i = 1; i < stmt.fromTables.size(); ++i) {
        QVector<Row> nextJoinedRows;
        auto innerRows = tables[i].fullScan();
        const auto& joinCond = stmt.fromTables[i].joinCondition;

        for (const auto& outerRow : joinedRows) {
            bool matched = false;
            for (const auto& innerPr : innerRows) {
                Row combined = outerRow;
                combined.append(innerPr.second);

                if (joinCond) {
                    if (Evaluator::evaluateCondition(*joinCond, combined, context)) {
                        nextJoinedRows.append(combined);
                        matched = true;
                    }
                } else {
                    nextJoinedRows.append(combined);
                    matched = true;
                }
            }
            if (!matched && stmt.fromTables[i].joinType == JoinType::LEFT) {
                Row combined = outerRow;
                for (int c = 0; c < schemas[i].second.columns.size(); ++c) {
                    combined.append(Value(std::monostate{}));
                }
                nextJoinedRows.append(combined);
            }
        }
        joinedRows = nextJoinedRows;
    }

    // WHERE Filter
    QVector<Row> filteredRows;
    if (stmt.whereClause) {
        for (const auto& row : joinedRows) {
            if (Evaluator::evaluateCondition(*stmt.whereClause, row, context)) {
                filteredRows.append(row);
            }
        }
    } else {
        filteredRows = joinedRows;
    }

    // Aggregations / GROUP BY
    bool hasAggregates = false;
    for (const auto& expr : stmt.columns) {
        if (std::holds_alternative<FunctionCallExpr>(expr)) hasAggregates = true;
    }

    QVector<Row> finalRows;
    if (hasAggregates || !stmt.groupBy.empty()) {
        std::map<QVector<Value>, QVector<Row>> groups;
        if (stmt.groupBy.empty()) {
            groups[{}] = filteredRows;
        } else {
            for (const auto& row : filteredRows) {
                QVector<Value> groupKey;
                for (const auto& gExpr : stmt.groupBy) {
                    groupKey.append(Evaluator::evaluate(gExpr, row, context));
                }
                groups[groupKey].append(row);
            }
        }

        for (const auto& [key, group] : groups) {
            Row outRow;
            for (const auto& expr : stmt.columns) {
                if (std::holds_alternative<FunctionCallExpr>(expr)) {
                    outRow.append(accumulateAggregate(std::get<FunctionCallExpr>(expr), group, context));
                } else {
                    outRow.append(Evaluator::evaluate(expr, group.empty() ? Row() : group[0], context));
                }
            }
            finalRows.append(outRow);
        }
    } else {
        for (const auto& row : filteredRows) {
            Row outRow;
            for (const auto& expr : stmt.columns) {
                outRow.append(Evaluator::evaluate(expr, row, context));
            }
            finalRows.append(outRow);
        }
    }
    
    if (stmt.isDistinct) {
        QVector<Row> uniqueRows;
        for (const auto& row : finalRows) {
            if (!uniqueRows.contains(row)) {
                uniqueRows.append(row);
            }
        }
        finalRows = uniqueRows;
    }

    if (!stmt.orderBy.empty()) {
        std::sort(finalRows.begin(), finalRows.end(), [&](const Row& a, const Row& b) {
            for (const auto& order : stmt.orderBy) {
                Value va = Evaluator::evaluate(order.expr, a, context);
                Value vb = Evaluator::evaluate(order.expr, b, context);
                if (va == vb) continue;
                if (order.direction == OrderDirection::ASC) return va < vb;
                else return va > vb;
            }
            return false;
        });
    }

    if (stmt.limit.has_value()) {
        int limit = stmt.limit.value();
        int offset = stmt.offset.value_or(0);
        QVector<Row> limited;
        for (int i = offset; i < finalRows.size() && limited.size() < limit; ++i) {
            limited.append(finalRows[i]);
        }
        finalRows = limited;
    }

    QueryResult result;
    for (const auto& expr : stmt.columns) {
        if (std::holds_alternative<ColumnRefExpr>(expr)) {
            result.columns.append(std::get<ColumnRefExpr>(expr).columnName);
        } else {
            result.columns.append("expr");
        }
    }
    result.rows = finalRows;
    return result;
}

QueryResult Executor::executeInsert(const InsertStmt& stmt) {
    if (!catalog.tableExists(stmt.tableName)) {
        throw Exception("Table does not exist: " + stmt.tableName);
    }
    TableSchema schema = catalog.getTableSchema(stmt.tableName);
    Table table(pager, schema.rootPage);
    BTree btree(pager, schema.rootPage);

    int count = 0;
    for (const auto& rowVals : stmt.values) {
        Row newRow;
        for (const auto& expr : rowVals) {
            newRow.append(Evaluator::evaluate(expr, Row(), schema));
        }
        
        int rowId = table.insertRow(newRow);
        
        if (schema.hasPrimaryKey) {
            int pkIndex = -1;
            for (int i = 0; i < schema.columns.size(); ++i) {
                if (schema.columns[i].name == schema.primaryKey) {
                    pkIndex = i;
                    break;
                }
            }
            if (pkIndex != -1) {
                btree.insert(newRow[pkIndex], rowId);
            }
        }
        count++;
    }

    return QueryResult::createResult(count);
}

QueryResult Executor::executeUpdate(const UpdateStmt& stmt) {
    if (!catalog.tableExists(stmt.tableName)) {
        throw Exception("Table does not exist: " + stmt.tableName);
    }
    TableSchema schema = catalog.getTableSchema(stmt.tableName);
    Table table(pager, schema.rootPage);
    BTree btree(pager, schema.rootPage);
    Planner planner(catalog);
    QueryPlan plan = planner.plan(stmt);

    auto allRows = table.fullScan();
    int count = 0;
    
    EvalContext context(schema);

    for (const auto& pr : allRows) {
        int rowId = pr.first;
        Row row = pr.second;
        
        if (!stmt.whereClause || Evaluator::evaluateCondition(*stmt.whereClause, row, context)) {
            Row updated = row;
            for (const auto& assign : stmt.assignments) {
                int colIdx = context.resolveColumn(std::nullopt, assign.first);
                updated[colIdx] = Evaluator::evaluate(assign.second, row, context);
            }
            
            table.updateRow(rowId, updated);
            
            if (schema.hasPrimaryKey) {
                int pkIndex = context.resolveColumn(std::nullopt, schema.primaryKey);
                if (row[pkIndex] != updated[pkIndex]) {
                    // btree.remove(row[pkIndex]);
                    btree.insert(updated[pkIndex], rowId);
                }
            }
            count++;
        }
    }
    
    return QueryResult::createResult(count);
}

QueryResult Executor::executeDelete(const DeleteStmt& stmt) {
    if (!catalog.tableExists(stmt.tableName)) {
        throw Exception("Table does not exist: " + stmt.tableName);
    }
    TableSchema schema = catalog.getTableSchema(stmt.tableName);
    Table table(pager, schema.rootPage);
    BTree btree(pager, schema.rootPage);
    Planner planner(catalog);
    QueryPlan plan = planner.plan(stmt);

    auto allRows = table.fullScan();
    int count = 0;
    
    EvalContext context(schema);

    for (const auto& pr : allRows) {
        int rowId = pr.first;
        Row row = pr.second;
        
        if (!stmt.whereClause || Evaluator::evaluateCondition(*stmt.whereClause, row, context)) {
            table.deleteRow(rowId);
            if (schema.hasPrimaryKey) {
                int pkIndex = context.resolveColumn(std::nullopt, schema.primaryKey);
                // btree.remove(row[pkIndex]);
            }
            count++;
        }
    }

    return QueryResult::createResult(count);
}

QueryResult Executor::executeCreateTable(const CreateTableStmt& stmt) {
    if (catalog.tableExists(stmt.tableName)) {
        throw Exception("Table already exists: " + stmt.tableName);
    }
    
    TableSchema schema;
    schema.tableName = stmt.tableName;
    for (const auto& cd : stmt.columns) {
        ColumnSchema cs;
        cs.name = cd.name;
        if (cd.type == DataType::INT) cs.type = ColumnType::INTEGER;
        else if (cd.type == DataType::VARCHAR) cs.type = ColumnType::VARCHAR;
        else if (cd.type == DataType::BOOLEAN) cs.type = ColumnType::BOOLEAN;
        schema.columns.append(cs);
        if (cd.isPrimaryKey) {
            schema.hasPrimaryKey = true;
            schema.primaryKey = cd.name;
        }
    }
    
    int rootPage = pager.allocatePage();
    schema.rootPage = rootPage;
    
    catalog.createTable(schema);
    
    return QueryResult::createResult(0);
}

QueryResult Executor::executeDropTable(const DropTableStmt& stmt) {
    if (!catalog.tableExists(stmt.tableName)) {
        throw Exception("Table does not exist: " + stmt.tableName);
    }
    catalog.dropTable(stmt.tableName);
    return QueryResult::createResult(0);
}

QueryResult Executor::executeShowTables(const ShowTablesStmt& /*stmt*/) {
    QueryResult result;
    result.columns.append("Tables");
    for (const auto& tName : catalog.getAllTableNames()) {
        Row r;
        r.append(Value(tName));
        result.rows.append(r);
    }
    return result;
}

QueryResult Executor::executeDescribe(const DescribeStmt& stmt) {
    if (!catalog.tableExists(stmt.tableName)) {
        throw Exception("Table does not exist: " + stmt.tableName);
    }
    TableSchema schema = catalog.getTableSchema(stmt.tableName);
    
    QueryResult result;
    result.columns.append("Field");
    result.columns.append("Type");
    result.columns.append("Key");
    
    for (const auto& col : schema.columns) {
        Row r;
        r.append(Value(col.name));
        QString typeStr = (col.type == ColumnType::INTEGER) ? "INTEGER" : 
                          (col.type == ColumnType::VARCHAR) ? "VARCHAR" : "BOOLEAN";
        r.append(Value(typeStr));
        r.append(Value(schema.hasPrimaryKey && schema.primaryKey == col.name ? "PRI" : ""));
        result.rows.append(r);
    }
    
    return result;
}

} // namespace minidb
