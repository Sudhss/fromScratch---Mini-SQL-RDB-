#include "engine/planner.h"
#include "core/errors.h"

namespace minidb {

Planner::Planner(Catalog& catalog) : catalog(catalog) {}

QueryPlan Planner::plan(const Statement& stmt) {
    if (const auto* s = dynamic_cast<const SelectStmt*>(&stmt)) return planSelect(*s);
    if (const auto* u = dynamic_cast<const UpdateStmt*>(&stmt)) return planUpdate(*u);
    if (const auto* d = dynamic_cast<const DeleteStmt*>(&stmt)) return planDelete(*d);
    return QueryPlan{};
}

void Planner::analyzeWhereForIndex(const Expression& where, const TableSchema& schema, QueryPlan& plan) {
    int pkIdx = schema.primaryKeyIndex();
    if (pkIdx == -1) return;
    QString pkName = schema.columns[pkIdx].name;

    if (const auto* binExpr = dynamic_cast<const BinaryExpr*>(&where)) {
        bool leftIsPk = false;
        bool rightIsPk = false;
        
        if (const auto* colL = dynamic_cast<const ColumnRefExpr*>(binExpr->left.get())) {
            if (colL->columnName == pkName) leftIsPk = true;
        }
        if (const auto* colR = dynamic_cast<const ColumnRefExpr*>(binExpr->right.get())) {
            if (colR->columnName == pkName) rightIsPk = true;
        }

        if (leftIsPk && dynamic_cast<const LiteralExpr*>(binExpr->right.get())) {
            Value val = dynamic_cast<const LiteralExpr*>(binExpr->right.get())->value;
            if (binExpr->op == TokenType::EQUALS) {
                plan.scanType = ScanType::INDEX_LOOKUP;
                plan.indexKey = val;
                plan.useIndex = true;
            } else if (binExpr->op == TokenType::GREATER_THAN || binExpr->op == TokenType::GREATER_THAN_EQUALS) {
                plan.scanType = ScanType::INDEX_RANGE_SCAN;
                plan.indexLow = val;
                plan.useIndex = true;
            } else if (binExpr->op == TokenType::LESS_THAN || binExpr->op == TokenType::LESS_THAN_EQUALS) {
                plan.scanType = ScanType::INDEX_RANGE_SCAN;
                plan.indexHigh = val;
                plan.useIndex = true;
            }
        }
    } else if (const auto* bwExpr = dynamic_cast<const BetweenExpr*>(&where)) {
        if (const auto* colExpr = dynamic_cast<const ColumnRefExpr*>(bwExpr->expr.get())) {
            if (colExpr->columnName == pkName) {
                if (dynamic_cast<const LiteralExpr*>(bwExpr->low.get()) && dynamic_cast<const LiteralExpr*>(bwExpr->high.get())) {
                    plan.scanType = ScanType::INDEX_RANGE_SCAN;
                    plan.indexLow = dynamic_cast<const LiteralExpr*>(bwExpr->low.get())->value;
                    plan.indexHigh = dynamic_cast<const LiteralExpr*>(bwExpr->high.get())->value;
                    plan.useIndex = true;
                }
            }
        }
    }
}

QueryPlan Planner::planSelect(const SelectStmt& stmt) {
    QueryPlan plan;
    if (stmt.tableRefs.empty()) return plan;
    
    // Simplistic JOIN order
    for (const auto& table : stmt.tableRefs) {
        plan.joinOrder.append(table.tableName);
    }
    
    // Analyze primary table for index usage if there's a WHERE clause
    if (stmt.whereClause) {
        const auto& mainTable = stmt.tableRefs[0].tableName;
        if (catalog.hasTable(mainTable)) {
            TableSchema schema = catalog.getSchema(mainTable);
            analyzeWhereForIndex(*stmt.whereClause, schema, plan);
        }
    }
    
    return plan;
}

QueryPlan Planner::planUpdate(const UpdateStmt& stmt) {
    QueryPlan plan;
    if (catalog.hasTable(stmt.tableName) && stmt.whereClause) {
        analyzeWhereForIndex(*stmt.whereClause, catalog.getSchema(stmt.tableName), plan);
    }
    return plan;
}

QueryPlan Planner::planDelete(const DeleteStmt& stmt) {
    QueryPlan plan;
    if (catalog.hasTable(stmt.tableName) && stmt.whereClause) {
        analyzeWhereForIndex(*stmt.whereClause, catalog.getSchema(stmt.tableName), plan);
    }
    return plan;
}

} // namespace minidb
