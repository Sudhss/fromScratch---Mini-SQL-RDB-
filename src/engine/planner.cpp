#include "engine/planner.h"
#include "core/errors.h"

namespace minidb {

Planner::Planner(Catalog& catalog) : catalog(catalog) {}

QueryPlan Planner::plan(const Statement& stmt) {
    return std::visit([&](auto&& arg) -> QueryPlan {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, SelectStmt>) return planSelect(arg);
        else if constexpr (std::is_same_v<T, UpdateStmt>) return planUpdate(arg);
        else if constexpr (std::is_same_v<T, DeleteStmt>) return planDelete(arg);
        else return QueryPlan{};
    }, stmt);
}

void Planner::analyzeWhereForIndex(const Expression& where, const TableSchema& schema, QueryPlan& plan) {
    if (!schema.hasPrimaryKey) return;

    if (std::holds_alternative<BinaryExpr>(where)) {
        const auto& binExpr = std::get<BinaryExpr>(where);
        
        bool leftIsPk = false;
        bool rightIsPk = false;
        
        if (std::holds_alternative<ColumnRefExpr>(*binExpr.left)) {
            if (std::get<ColumnRefExpr>(*binExpr.left).columnName == schema.primaryKey) leftIsPk = true;
        }
        if (std::holds_alternative<ColumnRefExpr>(*binExpr.right)) {
            if (std::get<ColumnRefExpr>(*binExpr.right).columnName == schema.primaryKey) rightIsPk = true;
        }

        if (leftIsPk && std::holds_alternative<LiteralExpr>(*binExpr.right)) {
            Value val = std::get<LiteralExpr>(*binExpr.right).value;
            if (binExpr.op == BinaryOperator::EQ) {
                plan.scanType = ScanType::INDEX_LOOKUP;
                plan.indexKey = val;
                plan.useIndex = true;
            } else if (binExpr.op == BinaryOperator::GT || binExpr.op == BinaryOperator::GTE) {
                plan.scanType = ScanType::INDEX_RANGE_SCAN;
                plan.indexLow = val;
                plan.useIndex = true;
            } else if (binExpr.op == BinaryOperator::LT || binExpr.op == BinaryOperator::LTE) {
                plan.scanType = ScanType::INDEX_RANGE_SCAN;
                plan.indexHigh = val;
                plan.useIndex = true;
            }
        }
    } else if (std::holds_alternative<BetweenExpr>(where)) {
        const auto& bwExpr = std::get<BetweenExpr>(where);
        if (std::holds_alternative<ColumnRefExpr>(*bwExpr.operand)) {
            if (std::get<ColumnRefExpr>(*bwExpr.operand).columnName == schema.primaryKey) {
                if (std::holds_alternative<LiteralExpr>(*bwExpr.lower) && std::holds_alternative<LiteralExpr>(*bwExpr.upper)) {
                    plan.scanType = ScanType::INDEX_RANGE_SCAN;
                    plan.indexLow = std::get<LiteralExpr>(*bwExpr.lower).value;
                    plan.indexHigh = std::get<LiteralExpr>(*bwExpr.upper).value;
                    plan.useIndex = true;
                }
            }
        }
    }
}

QueryPlan Planner::planSelect(const SelectStmt& stmt) {
    QueryPlan plan;
    if (stmt.fromTables.empty()) return plan;
    
    // Simplistic JOIN order
    for (const auto& table : stmt.fromTables) {
        plan.joinOrder.append(table.tableName);
    }
    
    // Analyze primary table for index usage if there's a WHERE clause
    if (stmt.whereClause) {
        const auto& mainTable = stmt.fromTables[0].tableName;
        if (catalog.tableExists(mainTable)) {
            TableSchema schema = catalog.getTableSchema(mainTable);
            analyzeWhereForIndex(*stmt.whereClause, schema, plan);
        }
    }
    
    return plan;
}

QueryPlan Planner::planUpdate(const UpdateStmt& stmt) {
    QueryPlan plan;
    if (catalog.tableExists(stmt.tableName) && stmt.whereClause) {
        analyzeWhereForIndex(*stmt.whereClause, catalog.getTableSchema(stmt.tableName), plan);
    }
    return plan;
}

QueryPlan Planner::planDelete(const DeleteStmt& stmt) {
    QueryPlan plan;
    if (catalog.tableExists(stmt.tableName) && stmt.whereClause) {
        analyzeWhereForIndex(*stmt.whereClause, catalog.getTableSchema(stmt.tableName), plan);
    }
    return plan;
}

} // namespace minidb
