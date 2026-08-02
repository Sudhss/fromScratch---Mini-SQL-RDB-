#pragma once

#include <QString>
#include <stdexcept>

namespace minidb {

// ── Base database error ───────────────────────────────────────────────

class DbError : public std::runtime_error {
public:
    explicit DbError(const QString& message)
        : std::runtime_error(message.toStdString()), m_message(message) {}

    const QString& message() const { return m_message; }

protected:
    QString m_message;
};

// ── Parse error (SQL syntax) ──────────────────────────────────────────

class ParseError : public DbError {
public:
    ParseError(const QString& message, int line = -1, int column = -1)
        : DbError(formatMessage(message, line, column))
        , m_line(line), m_column(column) {}

    int line() const   { return m_line; }
    int column() const { return m_column; }

private:
    int m_line;
    int m_column;

    static QString formatMessage(const QString& msg, int line, int col) {
        if (line >= 0 && col >= 0)
            return QString("Parse error at %1:%2: %3").arg(line).arg(col).arg(msg);
        if (line >= 0)
            return QString("Parse error at line %1: %2").arg(line).arg(msg);
        return QString("Parse error: %1").arg(msg);
    }
};

// ── Storage error (disk I/O, page corruption) ─────────────────────────

class StorageError : public DbError {
public:
    explicit StorageError(const QString& message)
        : DbError(QString("Storage error: %1").arg(message)) {}
};

// ── Execution error (runtime query errors) ────────────────────────────

class ExecutionError : public DbError {
public:
    explicit ExecutionError(const QString& message)
        : DbError(QString("Execution error: %1").arg(message)) {}
};

// ── Catalog error (schema violations) ─────────────────────────────────

class CatalogError : public DbError {
public:
    explicit CatalogError(const QString& message)
        : DbError(QString("Catalog error: %1").arg(message)) {}
};

// ── Constraint error (NOT NULL, PRIMARY KEY violations) ───────────────

class ConstraintError : public DbError {
public:
    explicit ConstraintError(const QString& message)
        : DbError(QString("Constraint violation: %1").arg(message)) {}
};

} // namespace minidb
