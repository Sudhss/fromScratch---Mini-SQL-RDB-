#pragma once

#include <QString>
#include <QVector>
#include <QPair>
#include <optional>
#include "core/types.h"
#include "sql/ast.h"

namespace minidb {

class EvalContext {
public:
    EvalContext(const TableSchema& schema);
    EvalContext(const QVector<QPair<QString, TableSchema>>& schemas);

    int resolveColumn(const std::optional<QString>& table, const QString& column) const;

    const QVector<QPair<QString, TableSchema>>& getSchemas() const { return schemas; }
    const QVector<int>& getColumnOffsets() const { return columnOffsets; }

private:
    QVector<QPair<QString, TableSchema>> schemas;
    QVector<int> columnOffsets;
};

class Evaluator {
public:
    static Value evaluate(const Expression& expr, const Row& row, const TableSchema& schema);
    static Value evaluate(const Expression& expr, const Row& combinedRow, const EvalContext& context);

    static bool evaluateCondition(const Expression& expr, const Row& row, const TableSchema& schema);
    static bool evaluateCondition(const Expression& expr, const Row& combinedRow, const EvalContext& context);

private:
    static Value evaluateInternal(const Expression& expr, const Row& row, const EvalContext& context);
    static bool likeMatch(const QString& pattern, const QString& text);
};

} // namespace minidb
