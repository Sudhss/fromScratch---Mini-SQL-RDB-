#include "engine/evaluator.h"
#include "core/errors.h"
#include <QRegularExpression>

namespace minidb {

EvalContext::EvalContext(const TableSchema& schema) {
    schemas.append({"", schema});
    columnOffsets.append(0);
}

EvalContext::EvalContext(const QVector<QPair<QString, TableSchema>>& schemas) : schemas(schemas) {
    int offset = 0;
    for (const auto& pair : schemas) {
        columnOffsets.append(offset);
        offset += pair.second.columns.size();
    }
}

int EvalContext::resolveColumn(const std::optional<QString>& table, const QString& column) const {
    int matchCount = 0;
    int matchedIndex = -1;

    for (int i = 0; i < schemas.size(); ++i) {
        const auto& tName = schemas[i].first;
        const auto& schema = schemas[i].second;

        if (table.has_value() && table.value() != tName) {
            continue;
        }

        for (int j = 0; j < schema.columns.size(); ++j) {
            if (schema.columns[j].name == column) {
                matchCount++;
                matchedIndex = columnOffsets[i] + j;
            }
        }
    }

    if (matchCount == 0) {
        throw Exception("Unknown column: " + column);
    }
    if (matchCount > 1) {
        throw Exception("Ambiguous column: " + column);
    }
    return matchedIndex;
}


Value Evaluator::evaluate(const Expression& expr, const Row& row, const TableSchema& schema) {
    EvalContext ctx(schema);
    return evaluateInternal(expr, row, ctx);
}

Value Evaluator::evaluate(const Expression& expr, const Row& combinedRow, const EvalContext& context) {
    return evaluateInternal(expr, combinedRow, context);
}

bool Evaluator::evaluateCondition(const Expression& expr, const Row& row, const TableSchema& schema) {
    Value val = evaluate(expr, row, schema);
    if (std::holds_alternative<std::monostate>(val)) return false;
    if (std::holds_alternative<int>(val)) return std::get<int>(val) != 0;
    return false;
}

bool Evaluator::evaluateCondition(const Expression& expr, const Row& combinedRow, const EvalContext& context) {
    Value val = evaluate(expr, combinedRow, context);
    if (std::holds_alternative<std::monostate>(val)) return false;
    if (std::holds_alternative<int>(val)) return std::get<int>(val) != 0;
    return false;
}

Value Evaluator::evaluateInternal(const Expression& expr, const Row& row, const EvalContext& context) {
    return std::visit([&](auto&& arg) -> Value {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, LiteralExpr>) {
            return arg.value;
        } else if constexpr (std::is_same_v<T, ColumnRefExpr>) {
            int idx = context.resolveColumn(arg.tableName, arg.columnName);
            return row[idx];
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            Value left = evaluateInternal(*arg.left, row, context);
            Value right = evaluateInternal(*arg.right, row, context);

            if (std::holds_alternative<std::monostate>(left) || std::holds_alternative<std::monostate>(right)) {
                if (arg.op == BinaryOperator::AND) {
                    if ((std::holds_alternative<int>(left) && std::get<int>(left) == 0) ||
                        (std::holds_alternative<int>(right) && std::get<int>(right) == 0)) {
                        return Value(0);
                    }
                } else if (arg.op == BinaryOperator::OR) {
                    if ((std::holds_alternative<int>(left) && std::get<int>(left) != 0) ||
                        (std::holds_alternative<int>(right) && std::get<int>(right) != 0)) {
                        return Value(1);
                    }
                }
                return Value(std::monostate{});
            }

            if (arg.op == BinaryOperator::AND) {
                bool l = std::get<int>(left) != 0;
                bool r = std::get<int>(right) != 0;
                return Value(l && r ? 1 : 0);
            } else if (arg.op == BinaryOperator::OR) {
                bool l = std::get<int>(left) != 0;
                bool r = std::get<int>(right) != 0;
                return Value(l || r ? 1 : 0);
            } else if (arg.op == BinaryOperator::EQ) {
                return Value(left == right ? 1 : 0);
            } else if (arg.op == BinaryOperator::NEQ) {
                return Value(left != right ? 1 : 0);
            } else if (arg.op == BinaryOperator::LT) {
                return Value(left < right ? 1 : 0);
            } else if (arg.op == BinaryOperator::GT) {
                return Value(left > right ? 1 : 0);
            } else if (arg.op == BinaryOperator::LTE) {
                return Value(left <= right ? 1 : 0);
            } else if (arg.op == BinaryOperator::GTE) {
                return Value(left >= right ? 1 : 0);
            } else if (arg.op == BinaryOperator::ADD) {
                if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                    return Value(std::get<int>(left) + std::get<int>(right));
                }
                throw Exception("Invalid types for +");
            } else if (arg.op == BinaryOperator::SUB) {
                if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                    return Value(std::get<int>(left) - std::get<int>(right));
                }
                throw Exception("Invalid types for -");
            } else if (arg.op == BinaryOperator::MUL) {
                if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                    return Value(std::get<int>(left) * std::get<int>(right));
                }
                throw Exception("Invalid types for *");
            } else if (arg.op == BinaryOperator::DIV) {
                if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
                    int r = std::get<int>(right);
                    if (r == 0) throw Exception("Division by zero");
                    return Value(std::get<int>(left) / r);
                }
                throw Exception("Invalid types for /");
            }
            throw Exception("Unsupported binary operator");
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            Value val = evaluateInternal(*arg.operand, row, context);
            if (std::holds_alternative<std::monostate>(val)) return Value(std::monostate{});
            if (arg.op == UnaryOperator::NOT) {
                return Value(std::get<int>(val) == 0 ? 1 : 0);
            } else if (arg.op == UnaryOperator::MINUS) {
                if (std::holds_alternative<int>(val)) {
                    return Value(-std::get<int>(val));
                }
                throw Exception("Invalid type for MINUS");
            }
            throw Exception("Unsupported unary operator");
        } else if constexpr (std::is_same_v<T, IsNullExpr>) {
            Value val = evaluateInternal(*arg.operand, row, context);
            bool isNull = std::holds_alternative<std::monostate>(val);
            return Value(arg.isNot ? (isNull ? 0 : 1) : (isNull ? 1 : 0));
        } else if constexpr (std::is_same_v<T, BetweenExpr>) {
            Value val = evaluateInternal(*arg.operand, row, context);
            Value low = evaluateInternal(*arg.lower, row, context);
            Value high = evaluateInternal(*arg.upper, row, context);
            if (std::holds_alternative<std::monostate>(val) || std::holds_alternative<std::monostate>(low) || std::holds_alternative<std::monostate>(high)) {
                return Value(std::monostate{});
            }
            return Value((val >= low && val <= high) ? 1 : 0);
        } else if constexpr (std::is_same_v<T, InListExpr>) {
            Value val = evaluateInternal(*arg.operand, row, context);
            if (std::holds_alternative<std::monostate>(val)) return Value(std::monostate{});
            bool found = false;
            for (const auto& listExpr : arg.list) {
                Value listVal = evaluateInternal(listExpr, row, context);
                if (val == listVal) {
                    found = true;
                    break;
                }
            }
            return Value(found ? 1 : 0);
        } else if constexpr (std::is_same_v<T, LikeExpr>) {
            Value val = evaluateInternal(*arg.operand, row, context);
            Value pat = evaluateInternal(*arg.pattern, row, context);
            if (std::holds_alternative<std::monostate>(val) || std::holds_alternative<std::monostate>(pat)) {
                return Value(std::monostate{});
            }
            if (std::holds_alternative<QString>(val) && std::holds_alternative<QString>(pat)) {
                return Value(likeMatch(std::get<QString>(pat), std::get<QString>(val)) ? 1 : 0);
            }
            throw Exception("LIKE requires string operands");
        } else if constexpr (std::is_same_v<T, FunctionCallExpr>) {
            // Function calls handled specially by executor, should not be evaluated normally here for aggregates
            throw Exception("Aggregates not evaluable directly here");
        }
        return Value(std::monostate{});
    }, expr);
}

bool Evaluator::likeMatch(const QString& pattern, const QString& text) {
    QString regexStr = "^";
    for (int i = 0; i < pattern.length(); ++i) {
        QChar c = pattern[i];
        if (c == '%') {
            regexStr += ".*";
        } else if (c == '_') {
            regexStr += ".";
        } else {
            regexStr += QRegularExpression::escape(QString(c));
        }
    }
    regexStr += "$";
    QRegularExpression re(regexStr, QRegularExpression::CaseInsensitiveOption);
    return re.match(text).hasMatch();
}

} // namespace minidb
