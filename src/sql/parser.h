#pragma once

#include <memory>
#include <QVector>
#include "token.h"
#include "ast.h"

namespace minidb {

class Parser {
public:
    explicit Parser(const QVector<Token>& tokens);
    std::unique_ptr<Statement> parse();

private:
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<Statement> parseSelect();
    std::unique_ptr<Statement> parseInsert();
    std::unique_ptr<Statement> parseUpdate();
    std::unique_ptr<Statement> parseDelete();
    std::unique_ptr<Statement> parseCreate();
    std::unique_ptr<Statement> parseDrop();
    std::unique_ptr<Statement> parseShowTables();
    std::unique_ptr<Statement> parseDescribe();

    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseOr();
    std::unique_ptr<Expression> parseAnd();
    std::unique_ptr<Expression> parseNot();
    std::unique_ptr<Expression> parseComparison();
    std::unique_ptr<Expression> parseAddition();
    std::unique_ptr<Expression> parseMultiplication();
    std::unique_ptr<Expression> parseUnary();
    std::unique_ptr<Expression> parsePrimary();

    DataType parseDataType();
    Value parseLiteral();
    
    // Helpers
    Token current() const;
    Token previous() const;
    Token advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token expect(TokenType type, const QString& message);

    QVector<Token> m_tokens;
    int m_current = 0;
};

} // namespace minidb
