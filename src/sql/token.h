#pragma once

#include <QString>
#include <QVector>

namespace minidb {

// ── Token Types ───────────────────────────────────────────────────────

enum class TokenType {
    // Literals
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    IDENTIFIER,

    // Keywords — DDL
    CREATE,
    TABLE,
    DROP,
    ALTER,
    ADD,
    COLUMN,

    // Keywords — DML
    SELECT,
    FROM,
    WHERE,
    INSERT,
    INTO,
    VALUES,
    UPDATE,
    SET,
    DELETE,

    // Keywords — Clauses
    ORDER,
    BY,
    ASC,
    DESC,
    LIMIT,
    GROUP,
    HAVING,
    AS,
    DISTINCT,

    // Keywords — Joins
    JOIN,
    INNER,
    LEFT,
    RIGHT,
    OUTER,
    ON,
    CROSS,

    // Keywords — Logical
    AND,
    OR,
    NOT,
    IN,
    BETWEEN,
    LIKE,
    IS,
    NULL_KEYWORD,    // NULL
    TRUE_KEYWORD,    // TRUE
    FALSE_KEYWORD,   // FALSE
    EXISTS,

    // Keywords — Data types
    INT_TYPE,
    FLOAT_TYPE,
    VARCHAR_TYPE,
    BOOL_TYPE,
    DATE_TYPE,
    TEXT_TYPE,

    // Keywords — Constraints
    PRIMARY,
    KEY,
    NOT_NULL,    // parsed as two tokens but recognized as constraint
    DEFAULT,
    UNIQUE,
    AUTOINCREMENT,
    AUTO_INCREMENT,

    // Keywords — Utility
    SHOW,
    TABLES,
    DESCRIBE,

    // Aggregate functions (recognized as keywords for highlighting)
    COUNT,
    SUM,
    AVG,
    MIN,
    MAX,

    // Operators
    EQUALS,             // =
    NOT_EQUALS,         // != or <>
    LESS_THAN,          // <
    GREATER_THAN,       // >
    LESS_THAN_EQUALS,   // <=
    GREATER_THAN_EQUALS,// >=
    PLUS,               // +
    MINUS,              // -
    ASTERISK,           // *
    SLASH,              // /
    PERCENT,            // %

    // Punctuation
    COMMA,              // ,
    SEMICOLON,          // ;
    LEFT_PAREN,         // (
    RIGHT_PAREN,        // )
    DOT,                // .

    // Special
    END_OF_INPUT,
    UNKNOWN,
};

// ── Token ─────────────────────────────────────────────────────────────

struct Token {
    TokenType   type  = TokenType::UNKNOWN;
    QString     value;
    int         line  = 0;
    int         col   = 0;

    Token() = default;
    Token(TokenType t, const QString& v, int ln, int c)
        : type(t), value(v), line(ln), col(c) {}

    bool is(TokenType t) const { return type == t; }
    bool isKeyword() const;
    bool isOperator() const;
    bool isLiteral() const;

    QString typeName() const;
};

QString tokenTypeName(TokenType type);

} // namespace minidb
