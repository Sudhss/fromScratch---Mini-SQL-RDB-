#pragma once

#include <QString>
#include <QVector>
#include <QHash>
#include "token.h"

namespace minidb {

class Lexer {
public:
    explicit Lexer(const QString& input);
    QVector<Token> tokenize();

private:
    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);
    bool isAtEnd() const;

    void scanToken();
    void identifier();
    void number();
    void stringLiteral();
    void blockComment();
    
    void addToken(TokenType type);
    void addToken(TokenType type, const QString& value);

    QString m_input;
    int m_start = 0;
    int m_current = 0;
    int m_line = 1;
    int m_colStart = 1;
    int m_colCurrent = 1;

    QVector<Token> m_tokens;
    static const QHash<QString, TokenType> s_keywords;
};

} // namespace minidb
