#include "engine/executor.h"
#include "storage/table.h"
#include "storage/btree.h"
#include "core/errors.h"
#include <algorithm>
#include <map>

namespace minidb {

Executor::Executor(Catalog& catalog, Pager& pager) : catalog(catalog), pager(pager) {}

QueryResult Executor::execute(const Statement& stmt) {
    if (const auto* s = dynamic_cast<const SelectStmt*>(&stmt)) return executeSelect(*s);
    if (const auto* i = dynamic_cast<const InsertStmt*>(&stmt)) return executeInsert(*i);
    if (const auto* u = dynamic_cast<const UpdateStmt*>(&stmt)) return executeUpdate(*u);
    if (const auto* d = dynamic_cast<const DeleteStmt*>(&stmt)) return executeDelete(*d);
    if (const auto* c = dynamic_cast<const CreateTableStmt*>(&stmt)) return executeCreateTable(*c);
    if (const auto* dt = dynamic_cast<const DropTableStmt*>(&stmt)) return executeDropTable(*dt);
    if (const auto* st = dynamic_cast<const ShowTablesStmt*>(&stmt)) return executeShowTables(*st);
    if (const auto* ds = dynamic_cast<const DescribeStmt*>(&stmt)) return executeDescribe(*ds);
    throw ExecutionError("Unsupported statement type");
}

Value Executor::accumulateAggregate(const FunctionCallExpr& func, const QVector<Row>& groupRows, const EvalContext& context) {
    QString fnName = func.functionName.toUpper();
    if (fnName == "COUNT") {
        if (func.args.empty()) {
            return Value(static_cast<int>(groupRows.size()));
        }
        int count = 0;
        for (const auto& row : groupRows) {
            Value val = Evaluator::evaluate(*func.args[0], row, context);
            if (!val.isNull()) {
                count++;
            }
        }
        return Value(count);
    } else if (fnName == "SUM") {
        int sum = 0;
        bool hasNonNull = false;
        for (const auto& row : groupRows) {
            Value val = Evaluator::evaluate(*func.args[0], row, context);
            if (val.type() == DataType::INT) {
                sum += val.toInt();
                hasNonNull = true;
            }
        }
        return hasNonNull ? Value(sum) : Value();
    } else if (fnName == "AVG") {
        int sum = 0;
        int count = 0;
        for (const auto& row : groupRows) {
            Value val = Evaluator::evaluate(*func.args[0], row, context);
            if (val.type() == DataType::INT) {
                sum += val.toInt();
                count++;
            }
        }
        return count > 0 ? Value(sum / count) : Value();
    } else if (fnName == "MIN") {
        std::optional<Value> minVal;
        for (const auto& row : groupRows) {
            Value val = Evaluator::evaluate(*func.args[0], row, context);
            if (!val.isNull()) {
                if (!minVal.has_value() || val < minVal.value()) {
                    minVal = val;
                }
            }
        }
        return minVal.has_value() ? minVal.value() : Value();
    } else if (fnName == "MAX") {
        std::optional<Value> maxVal;
        for (const auto& row : groupRows) {
            Value val = Evaluator::evaluate(*func.args[0], row, context);
            if (!val.isNull()) {
                if (!maxVal.has_value() || val > maxVal.value()) {
                    maxVal = val;
                }
            }
        }
        return maxVal.has_value() ? maxVal.value() : Value();
    }
    throw ExecutionError("Unsupported aggregate function: " + fnName);
}

QueryResult Executor::executeSelect(const SelectStmt& stmt) {
    if (stmt.tableRefs.empty()) {
        QStringList resCols;
        QVector<Row> resRows;
        Row row;
        for (const auto& expr : stmt.columns) {
            TableSchema emptySchema;
            emptySchema.name = "";
            row.push_back(Evaluator::evaluate(*expr.expression, Row(), emptySchema));
            resCols.append("expr");
        }
        resRows.append(row);
        return QueryResult::selectResult(resCols, resRows);
    }

    Planner planner(catalog);
    QueryPlan plan = planner.plan(stmt);

    QVector<QPair<QString, TableSchema>> schemas;
    std::vector<std::unique_ptr<Table>> tables;
    std::vector<std::unique_ptr<BTree>> btrees;

    for (const auto& tRef : stmt.tableRefs) {
        if (!catalog.hasTable(tRef.tableName)) {
            throw ExecutionError("Table does not exist: " + tRef.tableName);
        }
        TableSchema schema = catalog.getSchema(tRef.tableName);
        QString alias = tRef.alias.isEmpty() ? tRef.tableName : tRef.alias;
        schemas.append({alias, schema});
        
        tables.push_back(std::make_unique<Table>(pager, schema, catalog.getTablePageId(tRef.tableName)));
        btrees.push_back(std::make_unique<BTree>(pager, 0, DataType::INT));
    }

    EvalContext context(schemas);
    QVector<Row> joinedRows;

    // Scan the first table
    QVector<std::pair<RowId, Row>> firstTableRows;
    auto iter = tables[0]->scan();
    while (iter.hasNext()) firstTableRows.append(iter.next());
    
    if (plan.useIndex && plan.scanType == ScanType::INDEX_LOOKUP) {
        RowId rid = btrees[0]->search(plan.indexKey);
        if (rid.isValid()) {
            joinedRows.append(tables[0]->getRow(rid));
        }
    } else {
        for (const auto& pr : firstTableRows) joinedRows.append(pr.second);
    }

    // Process JOINs
    for (int i = 0; i < stmt.joins.size(); ++i) {
        const auto& join = stmt.joins[i];
        if (!catalog.hasTable(join.tableName)) throw ExecutionError("Table not found: " + join.tableName);
        
        TableSchema schema = catalog.getSchema(join.tableName);
        QString alias = join.alias.isEmpty() ? join.tableName : join.alias;
        schemas.append({alias, schema});
        
        Table joinTable(pager, schema, catalog.getTablePageId(join.tableName));
        
        QVector<Row> nextJoinedRows;
        auto innerRows = joinTable.scan();
        QVector<std::pair<RowId, Row>> innerRowsList;
        while (innerRows.hasNext()) innerRowsList.append(innerRows.next());
        
        for (const auto& outerRow : joinedRows) {
            bool matched = false;
            for (const auto& innerPr : innerRowsList) {
                Row combined = outerRow;
                combined.insert(combined.end(), innerPr.second.begin(), innerPr.second.end());

                if (join.onCondition) {
                    if (Evaluator::evaluateCondition(*join.onCondition, combined, context)) {
                        nextJoinedRows.append(combined);
                        matched = true;
                    }
                } else {
                    nextJoinedRows.append(combined);
                    matched = true;
                }
            }
            if (!matched && join.type == JoinType::LEFT) {
                Row combined = outerRow;
                for (int c = 0; c < schema.columns.size(); ++c) {
                    combined.push_back(Value());
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
        if (dynamic_cast<FunctionCallExpr*>(expr.expression.get())) hasAggregates = true;
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
                    groupKey.append(Evaluator::evaluate(*gExpr, row, context));
                }
                groups[groupKey].append(row);
            }
        }

        for (const auto& [key, group] : groups) {
            Row outRow;
            for (const auto& expr : stmt.columns) {
                if (auto* func = dynamic_cast<FunctionCallExpr*>(expr.expression.get())) {
                    outRow.push_back(accumulateAggregate(*func, group, context));
                } else {
                    outRow.push_back(Evaluator::evaluate(*expr.expression, group.empty() ? Row() : group[0], context));
                }
            }
            finalRows.append(outRow);
        }
    } else {
        for (const auto& row : filteredRows) {
            Row outRow;
            for (const auto& expr : stmt.columns) {
                outRow.push_back(Evaluator::evaluate(*expr.expression, row, context));
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
                Value va = Evaluator::evaluate(*order.expression, a, context);
                Value vb = Evaluator::evaluate(*order.expression, b, context);
                if (va == vb) continue;
                if (order.isAscending) return va < vb;
                else return va > vb;
            }
            return false;
        });
    }

    if (stmt.limit > 0) {
        int limit = stmt.limit;
        int offset = 0;
        QVector<Row> limited;
        for (int i = offset; i < finalRows.size() && limited.size() < limit; ++i) {
            limited.append(finalRows[i]);
        }
        finalRows = limited;
    }

    QStringList resColumns;
    for (const auto& expr : stmt.columns) {
        if (const auto* colRef = dynamic_cast<const ColumnRefExpr*>(expr.expression.get())) {
            resColumns.append(colRef->columnName);
        } else {
            resColumns.append("expr");
        }
    }
    
    return QueryResult::selectResult(resColumns, finalRows);
}

QueryResult Executor::executeInsert(const InsertStmt& stmt) {
    if (!catalog.hasTable(stmt.tableName)) {
        throw ExecutionError("Table does not exist: " + stmt.tableName);
    }
    TableSchema schema = catalog.getSchema(stmt.tableName);
    Table table(pager, schema, catalog.getTablePageId(stmt.tableName));

    int count = 0;
    for (const auto& rowVals : stmt.values) {
        RowId rowId = table.insertRow(rowVals);
        count++;
    }

    return QueryResult::modificationResult(count);
}

QueryResult Executor::executeUpdate(const UpdateStmt& stmt) {
    if (!catalog.hasTable(stmt.tableName)) {
        throw ExecutionError("Table does not exist: " + stmt.tableName);
    }
    TableSchema schema = catalog.getSchema(stmt.tableName);
    Table table(pager, schema, catalog.getTablePageId(stmt.tableName));
    Planner planner(catalog);
    QueryPlan plan = planner.plan(stmt);

    auto iter = table.scan();
    int count = 0;
    
    EvalContext context(schema);

    while (iter.hasNext()) {
        auto pr = iter.next();
        RowId rowId = pr.first;
        Row row = pr.second;
        
        if (!stmt.whereClause || Evaluator::evaluateCondition(*stmt.whereClause, row, context)) {
            Row updated = row;
            for (const auto& assign : stmt.setClauses) {
                int colIdx = context.resolveColumn(std::nullopt, assign.columnName);
                updated[colIdx] = Evaluator::evaluate(*assign.expression, row, context);
            }
            
            table.updateRow(rowId, updated);
            count++;
        }
    }
    
    return QueryResult::modificationResult(count);
}

QueryResult Executor::executeDelete(const DeleteStmt& stmt) {
    if (!catalog.hasTable(stmt.tableName)) {
        throw ExecutionError("Table does not exist: " + stmt.tableName);
    }
    TableSchema schema = catalog.getSchema(stmt.tableName);
    Table table(pager, schema, catalog.getTablePageId(stmt.tableName));
    Planner planner(catalog);
    QueryPlan plan = planner.plan(stmt);

    auto iter = table.scan();
    int count = 0;
    
    EvalContext context(schema);

    while (iter.hasNext()) {
        auto pr = iter.next();
        RowId rowId = pr.first;
        Row row = pr.second;
        
        if (!stmt.whereClause || Evaluator::evaluateCondition(*stmt.whereClause, row, context)) {
            table.deleteRow(rowId);
            count++;
        }
    }

    return QueryResult::modificationResult(count);
}

QueryResult Executor::executeCreateTable(const CreateTableStmt& stmt) {
    if (catalog.hasTable(stmt.tableName)) {
        throw ExecutionError("Table already exists: " + stmt.tableName);
    }
    
    TableSchema schema;
    schema.name = stmt.tableName;
    for (const auto& cd : stmt.columns) {
        schema.columns.push_back(cd);
    }
    
    catalog.createTable(schema);
    return QueryResult::ddlResult("Table created");
}

QueryResult Executor::executeDropTable(const DropTableStmt& stmt) {
    if (!catalog.hasTable(stmt.tableName)) {
        throw ExecutionError("Table does not exist: " + stmt.tableName);
    }
    catalog.dropTable(stmt.tableName);
    return QueryResult::ddlResult("Table dropped");
}

QueryResult Executor::executeShowTables(const ShowTablesStmt& /*stmt*/) {
    QStringList cols;
    cols.append("Tables");
    QVector<Row> rows;
    for (const auto& tName : catalog.listTables()) {
        Row r;
        r.push_back(Value(tName));
        rows.append(r);
    }
    return QueryResult::infoResult(cols, rows);
}

QueryResult Executor::executeDescribe(const DescribeStmt& stmt) {
    if (!catalog.hasTable(stmt.tableName)) {
        throw ExecutionError("Table does not exist: " + stmt.tableName);
    }
    TableSchema schema = catalog.getSchema(stmt.tableName);
    
    QStringList cols;
    cols.append("Field");
    cols.append("Type");
    cols.append("Key");
    
    QVector<Row> rows;
    for (const auto& col : schema.columns) {
        Row r;
        r.push_back(Value(col.name));
        QString typeStr = dataTypeToString(col.type);
        r.push_back(Value(typeStr));
        r.push_back(Value(col.isPrimaryKey() ? "PRI" : ""));
        rows.append(r);
    }
    
    return QueryResult::infoResult(cols, rows);
}

} // namespace minidb
