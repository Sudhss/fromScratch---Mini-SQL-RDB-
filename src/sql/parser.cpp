#include "parser.h"
#include "core/errors.h"
#include <QDate>

namespace minidb {

Parser::Parser(const QVector<Token>& tokens) : m_tokens(tokens) {}

std::unique_ptr<Statement> Parser::parse() {
    if (isAtEnd() || check(TokenType::END_OF_INPUT)) return nullptr;
    auto stmt = parseStatement();
    if (match(TokenType::SEMICOLON)) {
        // Optional trailing semicolon
    }
    return stmt;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (match(TokenType::SELECT)) return parseSelect();
    if (match(TokenType::INSERT)) return parseInsert();
    if (match(TokenType::UPDATE)) return parseUpdate();
    if (match(TokenType::DELETE)) return parseDelete();
    if (match(TokenType::CREATE)) return parseCreate();
    if (match(TokenType::DROP)) return parseDrop();
    if (match(TokenType::SHOW)) return parseShowTables();
    if (match(TokenType::DESCRIBE)) return parseDescribe();

    throw ParseError("Expected statement (SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, SHOW, DESCRIBE)", current().line, current().col);
}

std::unique_ptr<Statement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStmt>();

    if (match(TokenType::DISTINCT)) {
        stmt->isDistinct = true;
    }

    // Parse columns
    do {
        SelectColumn col;
        col.expression = parseExpression();
        if (match(TokenType::AS)) {
            col.alias = expect(TokenType::IDENTIFIER, "Expected alias after AS").value;
        } else if (check(TokenType::IDENTIFIER)) {
            col.alias = advance().value;
        }
        stmt->columns.push_back(std::move(col));
    } while (match(TokenType::COMMA));

    // Parse FROM
    if (match(TokenType::FROM)) {
        do {
            TableRef ref;
            ref.tableName = expect(TokenType::IDENTIFIER, "Expected table name").value;
            if (match(TokenType::AS)) {
                ref.alias = expect(TokenType::IDENTIFIER, "Expected alias after AS").value;
            } else if (check(TokenType::IDENTIFIER)) {
                ref.alias = advance().value;
            }
            stmt->tableRefs.push_back(ref);
        } while (match(TokenType::COMMA));

        // Parse JOINs
        while (check(TokenType::JOIN) || check(TokenType::INNER) || check(TokenType::LEFT) || check(TokenType::RIGHT) || check(TokenType::CROSS)) {
            JoinClause join;
            join.type = JoinType::INNER;

            if (match(TokenType::INNER)) {
                expect(TokenType::JOIN, "Expected JOIN after INNER");
            } else if (match(TokenType::LEFT)) {
                join.type = JoinType::LEFT;
                if (match(TokenType::OUTER)) {} // Optional
                expect(TokenType::JOIN, "Expected JOIN after LEFT");
            } else if (match(TokenType::RIGHT)) {
                join.type = JoinType::RIGHT;
                if (match(TokenType::OUTER)) {} // Optional
                expect(TokenType::JOIN, "Expected JOIN after RIGHT");
            } else if (match(TokenType::CROSS)) {
                join.type = JoinType::CROSS;
                expect(TokenType::JOIN, "Expected JOIN after CROSS");
            } else {
                expect(TokenType::JOIN, "Expected JOIN");
            }

            join.tableName = expect(TokenType::IDENTIFIER, "Expected table name in JOIN").value;
            if (match(TokenType::AS)) {
                join.alias = expect(TokenType::IDENTIFIER, "Expected alias after AS").value;
            } else if (check(TokenType::IDENTIFIER)) {
                if (current().value.toUpper() != "ON") {
                    join.alias = advance().value;
                }
            }

            if (join.type != JoinType::CROSS) {
                expect(TokenType::ON, "Expected ON clause for JOIN");
                join.onCondition = parseExpression();
            }

            stmt->joins.push_back(std::move(join));
        }
    }

    // Parse WHERE
    if (match(TokenType::WHERE)) {
        stmt->whereClause = parseExpression();
    }

    // Parse GROUP BY
    if (match(TokenType::GROUP)) {
        expect(TokenType::BY, "Expected BY after GROUP");
        do {
            stmt->groupBy.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }

    // Parse HAVING
    if (match(TokenType::HAVING)) {
        stmt->havingClause = parseExpression();
    }

    // Parse ORDER BY
    if (match(TokenType::ORDER)) {
        expect(TokenType::BY, "Expected BY after ORDER");
        do {
            OrderByItem item;
            item.expression = parseExpression();
            if (match(TokenType::DESC)) {
                item.isAscending = false;
            } else {
                match(TokenType::ASC); // Optional
                item.isAscending = true;
            }
            stmt->orderBy.push_back(std::move(item));
        } while (match(TokenType::COMMA));
    }

    // Parse LIMIT
    if (match(TokenType::LIMIT)) {
        stmt->limit = expect(TokenType::INTEGER_LITERAL, "Expected integer after LIMIT").value.toInt();
    }

    return stmt;
}

std::unique_ptr<Statement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStmt>();
    expect(TokenType::INTO, "Expected INTO after INSERT");
    stmt->tableName = expect(TokenType::IDENTIFIER, "Expected table name").value;

    if (match(TokenType::LEFT_PAREN)) {
        do {
            stmt->columns.push_back(expect(TokenType::IDENTIFIER, "Expected column name").value);
        } while (match(TokenType::COMMA));
        expect(TokenType::RIGHT_PAREN, "Expected ')' after column list");
    }

    expect(TokenType::VALUES, "Expected VALUES in INSERT statement");

    do {
        expect(TokenType::LEFT_PAREN, "Expected '(' for values");
        Row row;
        do {
            row.push_back(parseLiteral());
        } while (match(TokenType::COMMA));
        expect(TokenType::RIGHT_PAREN, "Expected ')' after values");
        stmt->values.push_back(row);
    } while (match(TokenType::COMMA));

    return stmt;
}

std::unique_ptr<Statement> Parser::parseUpdate() {
    auto stmt = std::make_unique<UpdateStmt>();
    stmt->tableName = expect(TokenType::IDENTIFIER, "Expected table name").value;

    expect(TokenType::SET, "Expected SET after table name");

    do {
        SetClause clause;
        clause.columnName = expect(TokenType::IDENTIFIER, "Expected column name").value;
        expect(TokenType::EQUALS, "Expected '=' after column name");
        clause.expression = parseExpression();
        stmt->setClauses.push_back(std::move(clause));
    } while (match(TokenType::COMMA));

    if (match(TokenType::WHERE)) {
        stmt->whereClause = parseExpression();
    }

    return stmt;
}

std::unique_ptr<Statement> Parser::parseDelete() {
    auto stmt = std::make_unique<DeleteStmt>();
    expect(TokenType::FROM, "Expected FROM after DELETE");
    stmt->tableName = expect(TokenType::IDENTIFIER, "Expected table name").value;

    if (match(TokenType::WHERE)) {
        stmt->whereClause = parseExpression();
    }

    return stmt;
}

std::unique_ptr<Statement> Parser::parseCreate() {
    expect(TokenType::TABLE, "Expected TABLE after CREATE");
    auto stmt = std::make_unique<CreateTableStmt>();
    stmt->tableName = expect(TokenType::IDENTIFIER, "Expected table name").value;

    expect(TokenType::LEFT_PAREN, "Expected '(' after table name");

    do {
        ColumnDef col;
        col.name = expect(TokenType::IDENTIFIER, "Expected column name").value;
        col.type = parseDataType();

        if (col.type == DataType::VARCHAR) {
            expect(TokenType::LEFT_PAREN, "Expected '(' for VARCHAR length");
            col.varcharMaxLen = expect(TokenType::INTEGER_LITERAL, "Expected length for VARCHAR").value.toInt();
            expect(TokenType::RIGHT_PAREN, "Expected ')' after VARCHAR length");
        }

        while (!check(TokenType::COMMA) && !check(TokenType::RIGHT_PAREN) && !isAtEnd()) {
            if (match(TokenType::PRIMARY)) {
                expect(TokenType::KEY, "Expected KEY after PRIMARY");
                col.constraints = col.constraints | Constraint::PRIMARY_KEY;
            } else if (match(TokenType::NOT)) {
                expect(TokenType::NULL_KEYWORD, "Expected NULL after NOT");
                col.constraints = col.constraints | Constraint::NOT_NULL;
            } else if (match(TokenType::DEFAULT)) {
                col.defaultValue = parseLiteral();
            } else {
                throw ParseError("Unexpected token in column definition", current().line, current().col);
            }
        }

        stmt->columns.push_back(col);
    } while (match(TokenType::COMMA));

    expect(TokenType::RIGHT_PAREN, "Expected ')' after column definitions");

    return stmt;
}

std::unique_ptr<Statement> Parser::parseDrop() {
    expect(TokenType::TABLE, "Expected TABLE after DROP");
    auto stmt = std::make_unique<DropTableStmt>();
    stmt->tableName = expect(TokenType::IDENTIFIER, "Expected table name").value;
    return stmt;
}

std::unique_ptr<Statement> Parser::parseShowTables() {
    expect(TokenType::TABLES, "Expected TABLES after SHOW");
    return std::make_unique<ShowTablesStmt>();
}

std::unique_ptr<Statement> Parser::parseDescribe() {
    auto stmt = std::make_unique<DescribeStmt>();
    stmt->tableName = expect(TokenType::IDENTIFIER, "Expected table name after DESCRIBE").value;
    return stmt;
}

// Expressions
std::unique_ptr<Expression> Parser::parseExpression() {
    return parseOr();
}

std::unique_ptr<Expression> Parser::parseOr() {
    auto expr = parseAnd();
    while (match(TokenType::OR)) {
        Token op = previous();
        auto right = parseAnd();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.type, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::parseAnd() {
    auto expr = parseNot();
    while (match(TokenType::AND)) {
        Token op = previous();
        auto right = parseNot();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.type, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::parseNot() {
    if (match(TokenType::NOT)) {
        Token op = previous();
        auto operand = parseComparison();
        return std::make_unique<UnaryExpr>(op.type, std::move(operand));
    }
    return parseComparison();
}

std::unique_ptr<Expression> Parser::parseComparison() {
    auto expr = parseAddition();

    if (match(TokenType::EQUALS) || match(TokenType::NOT_EQUALS) ||
        match(TokenType::LESS_THAN) || match(TokenType::GREATER_THAN) ||
        match(TokenType::LESS_THAN_EQUALS) || match(TokenType::GREATER_THAN_EQUALS)) {
        Token op = previous();
        auto right = parseAddition();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.type, std::move(right));
    } else if (match(TokenType::IS)) {
        bool isNot = false;
        if (match(TokenType::NOT)) {
            isNot = true;
        }
        expect(TokenType::NULL_KEYWORD, "Expected NULL after IS");
        expr = std::make_unique<IsNullExpr>(std::move(expr), isNot);
    } else if (match(TokenType::BETWEEN)) {
        auto low = parseAddition();
        expect(TokenType::AND, "Expected AND in BETWEEN clause");
        auto high = parseAddition();
        expr = std::make_unique<BetweenExpr>(std::move(expr), std::move(low), std::move(high));
    } else if (match(TokenType::NOT)) {
        if (match(TokenType::BETWEEN)) {
            auto low = parseAddition();
            expect(TokenType::AND, "Expected AND in BETWEEN clause");
            auto high = parseAddition();
            expr = std::make_unique<BetweenExpr>(std::move(expr), std::move(low), std::move(high), true);
        } else if (match(TokenType::IN)) {
            expect(TokenType::LEFT_PAREN, "Expected '(' after IN");
            std::vector<std::unique_ptr<Expression>> list;
            do {
                list.push_back(parseExpression());
            } while (match(TokenType::COMMA));
            expect(TokenType::RIGHT_PAREN, "Expected ')' after IN list");
            expr = std::make_unique<InListExpr>(std::move(expr), std::move(list), true);
        } else if (match(TokenType::LIKE)) {
            auto pattern = parseAddition();
            expr = std::make_unique<LikeExpr>(std::move(expr), std::move(pattern), true);
        } else {
            throw ParseError("Expected BETWEEN, IN, or LIKE after NOT", current().line, current().col);
        }
    } else if (match(TokenType::IN)) {
        expect(TokenType::LEFT_PAREN, "Expected '(' after IN");
        std::vector<std::unique_ptr<Expression>> list;
        do {
            list.push_back(parseExpression());
        } while (match(TokenType::COMMA));
        expect(TokenType::RIGHT_PAREN, "Expected ')' after IN list");
        expr = std::make_unique<InListExpr>(std::move(expr), std::move(list));
    } else if (match(TokenType::LIKE)) {
        auto pattern = parseAddition();
        expr = std::make_unique<LikeExpr>(std::move(expr), std::move(pattern));
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseAddition() {
    auto expr = parseMultiplication();
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        Token op = previous();
        auto right = parseMultiplication();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.type, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::parseMultiplication() {
    auto expr = parseUnary();
    while (match(TokenType::ASTERISK) || match(TokenType::SLASH) || match(TokenType::PERCENT)) {
        Token op = previous();
        auto right = parseUnary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.type, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::parseUnary() {
    if (match(TokenType::MINUS) || match(TokenType::PLUS)) {
        Token op = previous();
        auto right = parseUnary();
        return std::make_unique<UnaryExpr>(op.type, std::move(right));
    }
    return parsePrimary();
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    if (match(TokenType::INTEGER_LITERAL)) {
        return std::make_unique<LiteralExpr>(Value(previous().value.toInt()));
    }
    if (match(TokenType::FLOAT_LITERAL)) {
        return std::make_unique<LiteralExpr>(Value(previous().value.toDouble()));
    }
    if (match(TokenType::STRING_LITERAL)) {
        return std::make_unique<LiteralExpr>(Value(previous().value));
    }
    if (match(TokenType::TRUE_KEYWORD)) {
        return std::make_unique<LiteralExpr>(Value(true));
    }
    if (match(TokenType::FALSE_KEYWORD)) {
        return std::make_unique<LiteralExpr>(Value(false));
    }
    if (match(TokenType::NULL_KEYWORD)) {
        return std::make_unique<LiteralExpr>(Value());
    }
    if (match(TokenType::ASTERISK)) {
        return std::make_unique<StarExpr>();
    }

    if (match(TokenType::LEFT_PAREN)) {
        auto expr = parseExpression();
        expect(TokenType::RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }

    // Could be an identifier: column ref or function call
    if (match(TokenType::IDENTIFIER) || match(TokenType::COUNT) || match(TokenType::SUM) || match(TokenType::AVG) || match(TokenType::MIN) || match(TokenType::MAX)) {
        QString name = previous().value;

        if (match(TokenType::LEFT_PAREN)) {
            // Function call
            bool isDistinct = false;
            if (match(TokenType::DISTINCT)) {
                isDistinct = true;
            }
            std::vector<std::unique_ptr<Expression>> args;
            if (!check(TokenType::RIGHT_PAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::RIGHT_PAREN, "Expected ')' after function arguments");
            return std::make_unique<FunctionCallExpr>(name.toUpper(), std::move(args), isDistinct);
        } else if (match(TokenType::DOT)) {
            // Table.Column
            QString columnName = expect(TokenType::IDENTIFIER, "Expected column name after '.'").value;
            return std::make_unique<ColumnRefExpr>(columnName, name);
        } else {
            // Just a column name
            return std::make_unique<ColumnRefExpr>(name);
        }
    }

    throw ParseError("Expected expression", current().line, current().col);
}

DataType Parser::parseDataType() {
    if (match(TokenType::INT_TYPE)) return DataType::INT;
    if (match(TokenType::FLOAT_TYPE)) return DataType::FLOAT;
    if (match(TokenType::VARCHAR_TYPE)) return DataType::VARCHAR;
    if (match(TokenType::BOOL_TYPE)) return DataType::BOOL;
    if (match(TokenType::DATE_TYPE)) return DataType::DATE;
    
    // Some basic alternatives
    if (match(TokenType::IDENTIFIER)) {
        QString t = previous().value.toUpper();
        if (t == "INT" || t == "INTEGER") return DataType::INT;
        if (t == "FLOAT" || t == "DOUBLE") return DataType::FLOAT;
        if (t == "VARCHAR") return DataType::VARCHAR;
        if (t == "BOOL" || t == "BOOLEAN") return DataType::BOOL;
        if (t == "DATE") return DataType::DATE;
    }

    throw ParseError("Expected data type", current().line, current().col);
}

Value Parser::parseLiteral() {
    if (match(TokenType::INTEGER_LITERAL)) {
        return Value(previous().value.toInt());
    }
    if (match(TokenType::FLOAT_LITERAL)) {
        return Value(previous().value.toDouble());
    }
    if (match(TokenType::STRING_LITERAL)) {
        // Checking for DATE format 'YYYY-MM-DD'
        QString val = previous().value;
        if (val.count('-') == 2) {
            QDate d = QDate::fromString(val, "yyyy-MM-dd");
            if (d.isValid()) {
                return Value(d);
            }
        }
        return Value(val);
    }
    if (match(TokenType::TRUE_KEYWORD)) {
        return Value(true);
    }
    if (match(TokenType::FALSE_KEYWORD)) {
        return Value(false);
    }
    if (match(TokenType::NULL_KEYWORD)) {
        return Value();
    }
    if (match(TokenType::MINUS)) {
        if (match(TokenType::INTEGER_LITERAL)) {
            return Value(-previous().value.toInt());
        }
        if (match(TokenType::FLOAT_LITERAL)) {
            return Value(-previous().value.toDouble());
        }
    }

    throw ParseError("Expected literal value", current().line, current().col);
}

Token Parser::current() const {
    if (isAtEnd()) return m_tokens.last();
    return m_tokens[m_current];
}

Token Parser::previous() const {
    return m_tokens[m_current - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) m_current++;
    return previous();
}

bool Parser::isAtEnd() const {
    return m_current >= m_tokens.size() || m_tokens[m_current].type == TokenType::END_OF_INPUT;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return current().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::expect(TokenType type, const QString& message) {
    if (check(type)) return advance();
    throw ParseError(message, current().line, current().col);
}

} // namespace minidb
