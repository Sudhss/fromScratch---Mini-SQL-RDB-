#include "token.h"

namespace minidb {

bool Token::isKeyword() const {
    return type >= TokenType::CREATE && type <= TokenType::MAX;
}

bool Token::isOperator() const {
    return type >= TokenType::EQUALS && type <= TokenType::PERCENT;
}

bool Token::isLiteral() const {
    return type == TokenType::INTEGER_LITERAL
        || type == TokenType::FLOAT_LITERAL
        || type == TokenType::STRING_LITERAL;
}

QString Token::typeName() const {
    return tokenTypeName(type);
}

QString tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::INTEGER_LITERAL:     return "INTEGER_LITERAL";
        case TokenType::FLOAT_LITERAL:       return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL:      return "STRING_LITERAL";
        case TokenType::IDENTIFIER:          return "IDENTIFIER";
        case TokenType::CREATE:              return "CREATE";
        case TokenType::TABLE:               return "TABLE";
        case TokenType::DROP:                return "DROP";
        case TokenType::ALTER:               return "ALTER";
        case TokenType::ADD:                 return "ADD";
        case TokenType::COLUMN:              return "COLUMN";
        case TokenType::SELECT:              return "SELECT";
        case TokenType::FROM:                return "FROM";
        case TokenType::WHERE:               return "WHERE";
        case TokenType::INSERT:              return "INSERT";
        case TokenType::INTO:                return "INTO";
        case TokenType::VALUES:              return "VALUES";
        case TokenType::UPDATE:              return "UPDATE";
        case TokenType::SET:                 return "SET";
        case TokenType::DELETE:              return "DELETE";
        case TokenType::ORDER:               return "ORDER";
        case TokenType::BY:                  return "BY";
        case TokenType::ASC:                 return "ASC";
        case TokenType::DESC:                return "DESC";
        case TokenType::LIMIT:               return "LIMIT";
        case TokenType::GROUP:               return "GROUP";
        case TokenType::HAVING:              return "HAVING";
        case TokenType::AS:                  return "AS";
        case TokenType::DISTINCT:            return "DISTINCT";
        case TokenType::JOIN:                return "JOIN";
        case TokenType::INNER:               return "INNER";
        case TokenType::LEFT:                return "LEFT";
        case TokenType::RIGHT:               return "RIGHT";
        case TokenType::ON:                  return "ON";
        case TokenType::CROSS:               return "CROSS";
        case TokenType::AND:                 return "AND";
        case TokenType::OR:                  return "OR";
        case TokenType::NOT:                 return "NOT";
        case TokenType::IN:                  return "IN";
        case TokenType::BETWEEN:             return "BETWEEN";
        case TokenType::LIKE:                return "LIKE";
        case TokenType::IS:                  return "IS";
        case TokenType::NULL_KEYWORD:        return "NULL";
        case TokenType::TRUE_KEYWORD:        return "TRUE";
        case TokenType::FALSE_KEYWORD:       return "FALSE";
        case TokenType::EXISTS:              return "EXISTS";
        case TokenType::INT_TYPE:            return "INT";
        case TokenType::FLOAT_TYPE:          return "FLOAT";
        case TokenType::VARCHAR_TYPE:        return "VARCHAR";
        case TokenType::BOOL_TYPE:           return "BOOL";
        case TokenType::DATE_TYPE:           return "DATE";
        case TokenType::TEXT_TYPE:            return "TEXT";
        case TokenType::PRIMARY:             return "PRIMARY";
        case TokenType::KEY:                 return "KEY";
        case TokenType::NOT_NULL:            return "NOT_NULL";
        case TokenType::DEFAULT:             return "DEFAULT";
        case TokenType::UNIQUE:              return "UNIQUE";
        case TokenType::AUTOINCREMENT:       return "AUTOINCREMENT";
        case TokenType::AUTO_INCREMENT:      return "AUTO_INCREMENT";
        case TokenType::SHOW:                return "SHOW";
        case TokenType::TABLES:              return "TABLES";
        case TokenType::DESCRIBE:            return "DESCRIBE";
        case TokenType::COUNT:               return "COUNT";
        case TokenType::SUM:                 return "SUM";
        case TokenType::AVG:                 return "AVG";
        case TokenType::MIN:                 return "MIN";
        case TokenType::MAX:                 return "MAX";
        case TokenType::EQUALS:              return "=";
        case TokenType::NOT_EQUALS:          return "!=";
        case TokenType::LESS_THAN:           return "<";
        case TokenType::GREATER_THAN:        return ">";
        case TokenType::LESS_THAN_EQUALS:    return "<=";
        case TokenType::GREATER_THAN_EQUALS: return ">=";
        case TokenType::PLUS:                return "+";
        case TokenType::MINUS:               return "-";
        case TokenType::ASTERISK:            return "*";
        case TokenType::SLASH:               return "/";
        case TokenType::PERCENT:             return "%";
        case TokenType::COMMA:               return ",";
        case TokenType::SEMICOLON:           return ";";
        case TokenType::LEFT_PAREN:          return "(";
        case TokenType::RIGHT_PAREN:         return ")";
        case TokenType::DOT:                 return ".";
        case TokenType::END_OF_INPUT:        return "END";
        case TokenType::UNKNOWN:             return "UNKNOWN";
    }
    return "UNKNOWN";
}

} // namespace minidb
