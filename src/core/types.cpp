#include "types.h"

namespace minidb {

// ── DataType helpers ──────────────────────────────────────────────────

QString dataTypeToString(DataType type) {
    switch (type) {
        case DataType::INT:     return "INT";
        case DataType::FLOAT:   return "FLOAT";
        case DataType::VARCHAR: return "VARCHAR";
        case DataType::BOOL:    return "BOOL";
        case DataType::DATE:    return "DATE";
    }
    return "UNKNOWN";
}

DataType stringToDataType(const QString& str) {
    QString upper = str.toUpper().trimmed();
    if (upper == "INT" || upper == "INTEGER") return DataType::INT;
    if (upper == "FLOAT" || upper == "DOUBLE" || upper == "REAL") return DataType::FLOAT;
    if (upper.startsWith("VARCHAR") || upper == "TEXT" || upper == "STRING") return DataType::VARCHAR;
    if (upper == "BOOL" || upper == "BOOLEAN") return DataType::BOOL;
    if (upper == "DATE") return DataType::DATE;
    return DataType::VARCHAR;  // fallback
}

int fixedSize(DataType type) {
    switch (type) {
        case DataType::INT:     return 4;
        case DataType::FLOAT:   return 8;
        case DataType::BOOL:    return 1;
        case DataType::DATE:    return 4;
        case DataType::VARCHAR: return -1;  // variable-length
    }
    return -1;
}

// ── Value ─────────────────────────────────────────────────────────────

Value::Value() : m_data(std::monostate{}) {}
Value::Value(int32_t v) : m_data(v) {}
Value::Value(double v) : m_data(v) {}
Value::Value(const QString& v) : m_data(v) {}
Value::Value(bool v) : m_data(v) {}
Value::Value(const QDate& v) : m_data(v) {}

bool Value::isNull() const {
    return std::holds_alternative<std::monostate>(m_data);
}

DataType Value::type() const {
    return std::visit([](auto&& arg) -> DataType {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) return DataType::INT; // null, arbitrary
        else if constexpr (std::is_same_v<T, int32_t>)   return DataType::INT;
        else if constexpr (std::is_same_v<T, double>)    return DataType::FLOAT;
        else if constexpr (std::is_same_v<T, QString>)   return DataType::VARCHAR;
        else if constexpr (std::is_same_v<T, bool>)      return DataType::BOOL;
        else if constexpr (std::is_same_v<T, QDate>)     return DataType::DATE;
        else return DataType::INT;
    }, m_data);
}

int32_t Value::toInt() const     { return std::get<int32_t>(m_data); }
double  Value::toFloat() const   { return std::get<double>(m_data); }
QString Value::toVarchar() const { return std::get<QString>(m_data); }
bool    Value::toBool() const    { return std::get<bool>(m_data); }
QDate   Value::toDate() const    { return std::get<QDate>(m_data); }

QString Value::toString() const {
    if (isNull()) return "NULL";
    return std::visit([](auto&& arg) -> QString {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) return "NULL";
        else if constexpr (std::is_same_v<T, int32_t>)   return QString::number(arg);
        else if constexpr (std::is_same_v<T, double>)    return QString::number(arg, 'g', 10);
        else if constexpr (std::is_same_v<T, QString>)   return arg;
        else if constexpr (std::is_same_v<T, bool>)      return arg ? "TRUE" : "FALSE";
        else if constexpr (std::is_same_v<T, QDate>)     return arg.toString("yyyy-MM-dd");
        else return "?";
    }, m_data);
}

bool Value::operator==(const Value& other) const {
    if (isNull() || other.isNull()) return false;  // NULL != anything, even NULL
    return m_data == other.m_data;
}

bool Value::operator!=(const Value& other) const {
    if (isNull() || other.isNull()) return false;
    return m_data != other.m_data;
}

bool Value::operator<(const Value& other) const {
    if (isNull() || other.isNull()) return false;
    return m_data < other.m_data;
}

bool Value::operator<=(const Value& other) const {
    return *this < other || *this == other;
}

bool Value::operator>(const Value& other) const {
    if (isNull() || other.isNull()) return false;
    return other.m_data < m_data;
}

bool Value::operator>=(const Value& other) const {
    return *this > other || *this == other;
}

// ── TableSchema ───────────────────────────────────────────────────────

int TableSchema::findColumn(const QString& colName) const {
    for (int i = 0; i < static_cast<int>(columns.size()); ++i) {
        if (columns[i].name.compare(colName, Qt::CaseInsensitive) == 0)
            return i;
    }
    return -1;
}

int TableSchema::primaryKeyIndex() const {
    for (int i = 0; i < static_cast<int>(columns.size()); ++i) {
        if (columns[i].isPrimaryKey())
            return i;
    }
    return -1;
}

}
