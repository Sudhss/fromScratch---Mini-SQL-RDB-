#pragma once

#include "sql/ast.h"
#include "storage/catalog.h"
#include <QString>
#include <QVector>

namespace minidb {

enum class ScanType { FULL_SCAN, INDEX_LOOKUP, INDEX_RANGE_SCAN };

struct QueryPlan {
    ScanType scanType = ScanType::FULL_SCAN;
    Value indexKey;
    Value indexLow;
    Value indexHigh;
    bool useIndex = false;
    QVector<QString> joinOrder;
};

class Planner {
public:
    explicit Planner(Catalog& catalog);
    QueryPlan plan(const Statement& stmt);

private:
    Catalog& catalog;

    QueryPlan planSelect(const SelectStmt& stmt);
    QueryPlan planUpdate(const UpdateStmt& stmt);
    QueryPlan planDelete(const DeleteStmt& stmt);

    void analyzeWhereForIndex(const Expression& where, const TableSchema& schema, QueryPlan& plan);
};

} // namespace minidb
