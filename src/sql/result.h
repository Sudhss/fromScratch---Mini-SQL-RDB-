#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include "core/types.h"

namespace minidb {

class QueryResult {
public:
    enum class Type { SELECT, MODIFICATION, DDL, INFO };

    QueryResult() = default;

    static QueryResult selectResult(const QStringList& columns, const QVector<Row>& rows);
    static QueryResult modificationResult(int count);
    static QueryResult ddlResult(const QString& message);
    static QueryResult infoResult(const QStringList& columns, const QVector<Row>& rows);
    static QueryResult error(const QString& message);

    bool isSuccess() const { return m_success; }
    bool hasError() const { return !m_success; }
    QString errorMessage() const { return m_errorMessage; }
    Type type() const { return m_type; }

    int rowCount() const { return m_rows.size(); }
    int columnCount() const { return m_columns.size(); }
    
    QStringList columns() const { return m_columns; }
    QVector<Row> rows() const { return m_rows; }
    int affectedRows() const { return m_affectedRows; }
    QString message() const { return m_message; }

private:
    bool m_success = true;
    QString m_errorMessage;
    Type m_type = Type::SELECT;

    QStringList m_columns;
    QVector<Row> m_rows;
    int m_affectedRows = 0;
    QString m_message;
};

} // namespace minidb
