#pragma once

#include <memory>
#include <vector>
#include <QString>
#include "token.h"
#include "core/types.h"

namespace minidb {

class ASTNode {
public:
    virtual ~ASTNode() = default;
};

class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

class Statement : public ASTNode {
public:
    virtual ~Statement() = default;
};

// ── Expressions ────────────────────────────────────────────────────────

class LiteralExpr : public Expression {
public:
    explicit LiteralExpr(const Value& v) : value(v) {}
    Value value;
};

class ColumnRefExpr : public Expression {
public:
    ColumnRefExpr(const QString& c, const QString& t = "") : columnName(c), tableName(t) {}
    QString columnName;
    QString tableName;
};

class BinaryExpr : public Expression {
public:
    BinaryExpr(std::unique_ptr<Expression> l, TokenType o, std::unique_ptr<Expression> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
    std::unique_ptr<Expression> left;
    TokenType op;
    std::unique_ptr<Expression> right;
};

class UnaryExpr : public Expression {
public:
    UnaryExpr(TokenType o, std::unique_ptr<Expression> e) : op(o), operand(std::move(e)) {}
    TokenType op;
    std::unique_ptr<Expression> operand;
};

class FunctionCallExpr : public Expression {
public:
    FunctionCallExpr(const QString& n, std::vector<std::unique_ptr<Expression>> a, bool d = false)
        : functionName(n), args(std::move(a)), isDistinct(d) {}
    QString functionName;
    std::vector<std::unique_ptr<Expression>> args;
    bool isDistinct;
};

class StarExpr : public Expression {
public:
    StarExpr() = default;
};

class IsNullExpr : public Expression {
public:
    IsNullExpr(std::unique_ptr<Expression> e, bool n) : expr(std::move(e)), isNot(n) {}
    std::unique_ptr<Expression> expr;
    bool isNot;
};

class BetweenExpr : public Expression {
public:
    BetweenExpr(std::unique_ptr<Expression> e, std::unique_ptr<Expression> l, std::unique_ptr<Expression> h, bool n = false)
        : expr(std::move(e)), low(std::move(l)), high(std::move(h)), isNot(n) {}
    std::unique_ptr<Expression> expr;
    std::unique_ptr<Expression> low;
    std::unique_ptr<Expression> high;
    bool isNot;
};

class InListExpr : public Expression {
public:
    InListExpr(std::unique_ptr<Expression> e, std::vector<std::unique_ptr<Expression>> l, bool n = false)
        : expr(std::move(e)), list(std::move(l)), isNot(n) {}
    std::unique_ptr<Expression> expr;
    std::vector<std::unique_ptr<Expression>> list;
    bool isNot;
};

class LikeExpr : public Expression {
public:
    LikeExpr(std::unique_ptr<Expression> e, std::unique_ptr<Expression> p, bool n = false)
        : expr(std::move(e)), pattern(std::move(p)), isNot(n) {}
    std::unique_ptr<Expression> expr;
    std::unique_ptr<Expression> pattern;
    bool isNot;
};

// ── Support Structs ───────────────────────────────────────────────────

enum class JoinType { INNER, LEFT, RIGHT, CROSS };

struct JoinClause {
    JoinType type;
    QString tableName;
    QString alias;
    std::unique_ptr<Expression> onCondition;
};

struct OrderByItem {
    std::unique_ptr<Expression> expression;
    bool isAscending = true;
};

struct SelectColumn {
    std::unique_ptr<Expression> expression;
    QString alias;
};

struct TableRef {
    QString tableName;
    QString alias;
};

struct SetClause {
    QString columnName;
    std::unique_ptr<Expression> expression;
};

// ── Statements ────────────────────────────────────────────────────────

class SelectStmt : public Statement {
public:
    std::vector<SelectColumn> columns;
    std::vector<TableRef> tableRefs;
    std::vector<JoinClause> joins;
    std::unique_ptr<Expression> whereClause;
    std::vector<std::unique_ptr<Expression>> groupBy;
    std::unique_ptr<Expression> havingClause;
    std::vector<OrderByItem> orderBy;
    int limit = -1;
    bool isDistinct = false;
};

class InsertStmt : public Statement {
public:
    QString tableName;
    std::vector<QString> columns;
    std::vector<Row> values; 
};

class UpdateStmt : public Statement {
public:
    QString tableName;
    std::vector<SetClause> setClauses;
    std::unique_ptr<Expression> whereClause;
};

class DeleteStmt : public Statement {
public:
    QString tableName;
    std::unique_ptr<Expression> whereClause;
};

class CreateTableStmt : public Statement {
public:
    QString tableName;
    std::vector<ColumnDef> columns;
};

class DropTableStmt : public Statement {
public:
    QString tableName;
};

class ShowTablesStmt : public Statement {
};

class DescribeStmt : public Statement {
public:
    QString tableName;
};

} // namespace minidb
