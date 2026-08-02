#pragma once

#include <QString>
#include <QVariant>
#include <QDate>
#include <vector>
#include <variant>
#include <optional>
#include <cstdint>

namespace minidb {

// ── Data Types ────────────────────────────────────────────────────────

enum class DataType {
    INT,        // 4 bytes — int32_t
    FLOAT,      // 8 bytes — double
    VARCHAR,    // variable — UTF-8 string, max N chars
    BOOL,       // 1 byte
    DATE        // 4 bytes — days since epoch
};

QString dataTypeToString(DataType type);
DataType stringToDataType(const QString& str);
int fixedSize(DataType type);  // returns -1 for variable-length types

// ── Value ─────────────────────────────────────────────────────────────
// A tagged union holding any supported column value, or null.

using ValueData = std::variant<
    std::monostate,     // NULL
    int32_t,            // INT
    double,             // FLOAT
    QString,            // VARCHAR
    bool,               // BOOL
    QDate               // DATE
>;

class Value {
public:
    Value();                                    // NULL value
    explicit Value(int32_t v);
    explicit Value(double v);
    explicit Value(const QString& v);
    explicit Value(bool v);
    explicit Value(const QDate& v);

    bool isNull() const;
    DataType type() const;

    int32_t     toInt() const;
    double      toFloat() const;
    QString     toVarchar() const;
    bool        toBool() const;
    QDate       toDate() const;

    // Display string for GUI / CLI output
    QString toString() const;

    // Comparison operators
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const;
    bool operator<(const Value& other) const;
    bool operator<=(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator>=(const Value& other) const;

    const ValueData& data() const { return m_data; }

private:
    ValueData m_data;
};

// ── Column Definition ─────────────────────────────────────────────────

enum class Constraint : uint8_t {
    NONE        = 0x00,
    PRIMARY_KEY = 0x01,
    NOT_NULL    = 0x02,
};

// Allow bitwise OR on Constraint
inline Constraint operator|(Constraint a, Constraint b) {
    return static_cast<Constraint>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
    );
}

inline bool hasConstraint(Constraint flags, Constraint check) {
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(check)) != 0;
}

struct ColumnDef {
    QString     name;
    DataType    type;
    int         varcharMaxLen = 0;   // only meaningful for VARCHAR
    Constraint  constraints = Constraint::NONE;
    std::optional<Value> defaultValue;

    bool isPrimaryKey() const { return hasConstraint(constraints, Constraint::PRIMARY_KEY); }
    bool isNotNull() const   { return hasConstraint(constraints, Constraint::NOT_NULL); }
};

// ── Table Schema ──────────────────────────────────────────────────────

struct TableSchema {
    QString                 name;
    std::vector<ColumnDef>  columns;

    int findColumn(const QString& colName) const;   // returns index or -1
    int primaryKeyIndex() const;                     // returns index or -1
};

// ── Row ───────────────────────────────────────────────────────────────

using Row = std::vector<Value>;

// ── Row ID ────────────────────────────────────────────────────────────
// Uniquely identifies a row: (page number, slot index within page)

struct RowId {
    uint32_t pageId = 0;
    uint16_t slotIndex = 0;

    bool operator==(const RowId& other) const {
        return pageId == other.pageId && slotIndex == other.slotIndex;
    }
    bool operator!=(const RowId& other) const { return !(*this == other); }
    bool operator<(const RowId& other) const {
        if (pageId != other.pageId) return pageId < other.pageId;
        return slotIndex < other.slotIndex;
    }

    static RowId invalid() { return { UINT32_MAX, UINT16_MAX }; }
    bool isValid() const { return pageId != UINT32_MAX; }
};

} // namespace minidb
