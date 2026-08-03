#pragma once

#include "sql/ast.h"
#include "sql/result.h"
#include "storage/catalog.h"
#include "storage/pager.h"
#include "engine/planner.h"
#include "engine/evaluator.h"
#include <QVector>

namespace minidb {

class Executor {
public:
    Executor(Catalog& catalog, Pager& pager);
    QueryResult execute(const Statement& stmt);

private:
    Catalog& catalog;
    Pager& pager;

    QueryResult executeSelect(const SelectStmt& stmt);
    QueryResult executeInsert(const InsertStmt& stmt);
    QueryResult executeUpdate(const UpdateStmt& stmt);
    QueryResult executeDelete(const DeleteStmt& stmt);
    QueryResult executeCreateTable(const CreateTableStmt& stmt);
    QueryResult executeDropTable(const DropTableStmt& stmt);
    QueryResult executeShowTables(const ShowTablesStmt& stmt);
    QueryResult executeDescribe(const DescribeStmt& stmt);
    
    Value accumulateAggregate(const FunctionCallExpr& func, const QVector<Row>& groupRows, const EvalContext& context);
};

} // namespace minidb
