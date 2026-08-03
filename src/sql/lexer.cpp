#include "lexer.h"
#include <cctype>

namespace minidb {

const QHash<QString, TokenType> Lexer::s_keywords = {
    {"CREATE", TokenType::CREATE}, {"TABLE", TokenType::TABLE}, {"DROP", TokenType::DROP},
    {"ALTER", TokenType::ALTER}, {"ADD", TokenType::ADD}, {"COLUMN", TokenType::COLUMN},
    {"SELECT", TokenType::SELECT}, {"FROM", TokenType::FROM}, {"WHERE", TokenType::WHERE},
    {"INSERT", TokenType::INSERT}, {"INTO", TokenType::INTO}, {"VALUES", TokenType::VALUES},
    {"UPDATE", TokenType::UPDATE}, {"SET", TokenType::SET}, {"DELETE", TokenType::DELETE},
    {"ORDER", TokenType::ORDER}, {"BY", TokenType::BY}, {"ASC", TokenType::ASC},
    {"DESC", TokenType::DESC}, {"LIMIT", TokenType::LIMIT}, {"GROUP", TokenType::GROUP},
    {"HAVING", TokenType::HAVING}, {"AS", TokenType::AS}, {"DISTINCT", TokenType::DISTINCT},
    {"JOIN", TokenType::JOIN}, {"INNER", TokenType::INNER}, {"LEFT", TokenType::LEFT},
    {"RIGHT", TokenType::RIGHT}, {"ON", TokenType::ON}, {"CROSS", TokenType::CROSS},
    {"AND", TokenType::AND}, {"OR", TokenType::OR}, {"NOT", TokenType::NOT},
    {"IN", TokenType::IN}, {"BETWEEN", TokenType::BETWEEN}, {"LIKE", TokenType::LIKE},
    {"IS", TokenType::IS}, {"NULL", TokenType::NULL_KEYWORD}, {"TRUE", TokenType::TRUE_KEYWORD},
    {"FALSE", TokenType::FALSE_KEYWORD}, {"EXISTS", TokenType::EXISTS},
    {"INT", TokenType::INT_TYPE}, {"FLOAT", TokenType::FLOAT_TYPE},
    {"VARCHAR", TokenType::VARCHAR_TYPE}, {"BOOL", TokenType::BOOL_TYPE},
    {"DATE", TokenType::DATE_TYPE}, {"TEXT", TokenType::TEXT_TYPE},
    {"PRIMARY", TokenType::PRIMARY}, {"KEY", TokenType::KEY}, {"DEFAULT", TokenType::DEFAULT},
    {"UNIQUE", TokenType::UNIQUE}, {"AUTOINCREMENT", TokenType::AUTOINCREMENT},
    {"AUTO_INCREMENT", TokenType::AUTO_INCREMENT},
    {"SHOW", TokenType::SHOW}, {"TABLES", TokenType::TABLES}, {"DESCRIBE", TokenType::DESCRIBE},
    {"COUNT", TokenType::COUNT}, {"SUM", TokenType::SUM}, {"AVG", TokenType::AVG},
    {"MIN", TokenType::MIN}, {"MAX", TokenType::MAX}
};

Lexer::Lexer(const QString& input) : m_input(input) {}

QVector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        m_start = m_current;
        m_colStart = m_colCurrent;
        scanToken();
    }
    m_start = m_current;
    m_colStart = m_colCurrent;
    addToken(TokenType::END_OF_INPUT, "");
    return m_tokens;
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return m_input[m_current].toLatin1();
}

char Lexer::peekNext() const {
    if (m_current + 1 >= m_input.length()) return '\0';
    return m_input[m_current + 1].toLatin1();
}

char Lexer::advance() {
    m_colCurrent++;
    return m_input[m_current++].toLatin1();
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (m_input[m_current].toLatin1() != expected) return false;
    m_current++;
    m_colCurrent++;
    return true;
}

bool Lexer::isAtEnd() const {
    return m_current >= m_input.length();
}

void Lexer::scanToken() {
    char c = advance();
    switch (c) {
        case '(': addToken(TokenType::LEFT_PAREN, "("); break;
        case ')': addToken(TokenType::RIGHT_PAREN, ")"); break;
        case ',': addToken(TokenType::COMMA, ","); break;
        case '.': addToken(TokenType::DOT, "."); break;
        case ';': addToken(TokenType::SEMICOLON, ";"); break;
        case '+': addToken(TokenType::PLUS, "+"); break;
        case '-':
            if (match('-')) {
                while (peek() != '\n' && !isAtEnd()) advance();
            } else {
                addToken(TokenType::MINUS, "-");
            }
            break;
        case '*':
            addToken(TokenType::ASTERISK, "*");
            break;
        case '/':
            if (match('*')) {
                blockComment();
            } else {
                addToken(TokenType::SLASH, "/");
            }
            break;
        case '%': addToken(TokenType::PERCENT, "%"); break;
        case '!':
            if (match('=')) {
                addToken(TokenType::NOT_EQUALS, "!=");
            } else {
                addToken(TokenType::UNKNOWN, "!");
            }
            break;
        case '=':
            addToken(TokenType::EQUALS, "=");
            break;
        case '<':
            if (match('=')) {
                addToken(TokenType::LESS_THAN_EQUALS, "<=");
            } else if (match('>')) {
                addToken(TokenType::NOT_EQUALS, "<>");
            } else {
                addToken(TokenType::LESS_THAN, "<");
            }
            break;
        case '>':
            if (match('=')) {
                addToken(TokenType::GREATER_THAN_EQUALS, ">=");
            } else {
                addToken(TokenType::GREATER_THAN, ">");
            }
            break;
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            m_line++;
            m_colCurrent = 1;
            break;
        case '\'':
            stringLiteral();
            break;
        default:
            if (std::isdigit(c)) {
                number();
            } else if (std::isalpha(c) || c == '_') {
                identifier();
            } else {
                addToken(TokenType::UNKNOWN, QString(c));
            }
            break;
    }
}

void Lexer::blockComment() {
    while (!isAtEnd()) {
        if (peek() == '*' && peekNext() == '/') {
            advance();
            advance();
            return;
        }
        if (peek() == '\n') {
            m_line++;
            m_colCurrent = 1;
        }
        advance();
    }
}

void Lexer::identifier() {
    while (std::isalnum(peek()) || peek() == '_') {
        advance();
    }

    QString text = m_input.mid(m_start, m_current - m_start);
    QString upperText = text.toUpper();

    TokenType type = TokenType::IDENTIFIER;
    if (s_keywords.contains(upperText)) {
        type = s_keywords.value(upperText);
    }
    
    // For NOT NULL constraint parsing support, wait, NOT_NULL token is not in keywords usually, 
    // it says "parsed as two tokens but recognized as constraint" in token.h
    addToken(type, text);
}

void Lexer::number() {
    bool isFloat = false;
    while (std::isdigit(peek())) {
        advance();
    }

    if (peek() == '.' && std::isdigit(peekNext())) {
        isFloat = true;
        advance();
        while (std::isdigit(peek())) {
            advance();
        }
    }

    QString text = m_input.mid(m_start, m_current - m_start);
    addToken(isFloat ? TokenType::FLOAT_LITERAL : TokenType::INTEGER_LITERAL, text);
}

void Lexer::stringLiteral() {
    while (peek() != '\'' && !isAtEnd()) {
        if (peek() == '\n') {
            m_line++;
            m_colCurrent = 1;
        }
        advance();
    }

    if (isAtEnd()) {
        addToken(TokenType::UNKNOWN, m_input.mid(m_start, m_current - m_start));
        return;
    }

    advance(); // closing quote

    // The value should not include the surrounding quotes
    QString value = m_input.mid(m_start + 1, m_current - m_start - 2);
    addToken(TokenType::STRING_LITERAL, value);
}

void Lexer::addToken(TokenType type) {
    addToken(type, m_input.mid(m_start, m_current - m_start));
}

void Lexer::addToken(TokenType type, const QString& value) {
    m_tokens.append(Token(type, value, m_line, m_colStart));
}

} // namespace minidb
