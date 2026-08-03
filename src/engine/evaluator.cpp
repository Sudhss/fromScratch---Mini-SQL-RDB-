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
        throw ExecutionError("Unknown column: " + column);
    }
    if (matchCount > 1) {
        throw ExecutionError("Ambiguous column: " + column);
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
    if (val.isNull()) return false;
    if (val.type() == DataType::INT) return val.toInt() != 0;
    return false;
}

bool Evaluator::evaluateCondition(const Expression& expr, const Row& combinedRow, const EvalContext& context) {
    Value val = evaluate(expr, combinedRow, context);
    if (val.isNull()) return false;
    if (val.type() == DataType::INT) return val.toInt() != 0;
    return false;
}

Value Evaluator::evaluateInternal(const Expression& expr, const Row& row, const EvalContext& context) {
    if (const auto* arg = dynamic_cast<const LiteralExpr*>(&expr)) {
        return arg->value;
    } else if (const auto* arg = dynamic_cast<const ColumnRefExpr*>(&expr)) {
        int idx = context.resolveColumn(arg->tableName, arg->columnName);
        return row[idx];
    } else if (const auto* arg = dynamic_cast<const BinaryExpr*>(&expr)) {
        Value left = evaluateInternal(*arg->left, row, context);
        Value right = evaluateInternal(*arg->right, row, context);

        if (left.isNull() || right.isNull()) {
            if (arg->op == TokenType::AND) {
                if ((left.type() == DataType::INT && left.toInt() == 0) ||
                    (right.type() == DataType::INT && right.toInt() == 0)) {
                    return Value(0);
                }
            } else if (arg->op == TokenType::OR) {
                if ((left.type() == DataType::INT && left.toInt() != 0) ||
                    (right.type() == DataType::INT && right.toInt() != 0)) {
                    return Value(1);
                }
            }
            return Value();
        }

        if (arg->op == TokenType::AND) {
            bool l = left.toInt() != 0;
            bool r = right.toInt() != 0;
            return Value(l && r ? 1 : 0);
        } else if (arg->op == TokenType::OR) {
            bool l = left.toInt() != 0;
            bool r = right.toInt() != 0;
            return Value(l || r ? 1 : 0);
        } else if (arg->op == TokenType::EQUALS) {
            return Value(left == right ? 1 : 0);
        } else if (arg->op == TokenType::NOT_EQUALS) {
            return Value(left != right ? 1 : 0);
        } else if (arg->op == TokenType::LESS_THAN) {
            return Value(left < right ? 1 : 0);
        } else if (arg->op == TokenType::GREATER_THAN) {
            return Value(left > right ? 1 : 0);
        } else if (arg->op == TokenType::LESS_THAN_EQUALS) {
            return Value(left <= right ? 1 : 0);
        } else if (arg->op == TokenType::GREATER_THAN_EQUALS) {
            return Value(left >= right ? 1 : 0);
        } else if (arg->op == TokenType::PLUS) {
            if (left.type() == DataType::INT && right.type() == DataType::INT) {
                return Value(left.toInt() + right.toInt());
            }
            throw ExecutionError("Invalid types for +");
        } else if (arg->op == TokenType::MINUS) {
            if (left.type() == DataType::INT && right.type() == DataType::INT) {
                return Value(left.toInt() - right.toInt());
            }
            throw ExecutionError("Invalid types for -");
        } else if (arg->op == TokenType::ASTERISK) {
            if (left.type() == DataType::INT && right.type() == DataType::INT) {
                return Value(left.toInt() * right.toInt());
            }
            throw ExecutionError("Invalid types for *");
        } else if (arg->op == TokenType::SLASH) {
            if (left.type() == DataType::INT && right.type() == DataType::INT) {
                int r = right.toInt();
                if (r == 0) throw ExecutionError("Division by zero");
                return Value(left.toInt() / r);
            }
            throw ExecutionError("Invalid types for /");
        }
        throw ExecutionError("Unsupported binary operator");
    } else if (const auto* arg = dynamic_cast<const UnaryExpr*>(&expr)) {
        Value val = evaluateInternal(*arg->operand, row, context);
        if (val.isNull()) return Value();
        if (arg->op == TokenType::NOT) {
            return Value(val.toInt() == 0 ? 1 : 0);
        } else if (arg->op == TokenType::MINUS) {
            if (val.type() == DataType::INT) {
                return Value(-val.toInt());
            }
            throw ExecutionError("Invalid type for MINUS");
        }
        throw ExecutionError("Unsupported unary operator");
    } else if (const auto* arg = dynamic_cast<const IsNullExpr*>(&expr)) {
        Value val = evaluateInternal(*arg->expr, row, context);
        bool isNull = val.isNull();
        return Value(arg->isNot ? (isNull ? 0 : 1) : (isNull ? 1 : 0));
    } else if (const auto* arg = dynamic_cast<const BetweenExpr*>(&expr)) {
        Value val = evaluateInternal(*arg->expr, row, context);
        Value low = evaluateInternal(*arg->low, row, context);
        Value high = evaluateInternal(*arg->high, row, context);
        if (val.isNull() || low.isNull() || high.isNull()) {
            return Value();
        }
        return Value((val >= low && val <= high) ? 1 : 0);
    } else if (const auto* arg = dynamic_cast<const InListExpr*>(&expr)) {
        Value val = evaluateInternal(*arg->expr, row, context);
        if (val.isNull()) return Value();
        bool found = false;
        for (const auto& listExpr : arg->list) {
            Value listVal = evaluateInternal(*listExpr, row, context);
            if (val == listVal) {
                found = true;
                break;
            }
        }
        return Value(arg->isNot ? (found ? 0 : 1) : (found ? 1 : 0));
    } else if (const auto* arg = dynamic_cast<const LikeExpr*>(&expr)) {
        Value val = evaluateInternal(*arg->expr, row, context);
        Value pat = evaluateInternal(*arg->pattern, row, context);
        if (val.isNull() || pat.isNull()) {
            return Value();
        }
        if (val.type() == DataType::VARCHAR && pat.type() == DataType::VARCHAR) {
            bool matches = likeMatch(pat.toVarchar(), val.toVarchar());
            return Value(arg->isNot ? (matches ? 0 : 1) : (matches ? 1 : 0));
        }
        throw ExecutionError("LIKE requires string operands");
    } else if (dynamic_cast<const FunctionCallExpr*>(&expr)) {
        throw ExecutionError("Aggregates not evaluable directly here");
    }
    return Value();
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
