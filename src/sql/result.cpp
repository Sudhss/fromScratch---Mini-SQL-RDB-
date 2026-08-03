#include "result.h"

namespace minidb {

QueryResult QueryResult::selectResult(const QStringList& columns, const QVector<Row>& rows) {
    QueryResult res;
    res.m_type = Type::SELECT;
    res.m_columns = columns;
    res.m_rows = rows;
    return res;
}

QueryResult QueryResult::modificationResult(int count) {
    QueryResult res;
    res.m_type = Type::MODIFICATION;
    res.m_affectedRows = count;
    return res;
}

QueryResult QueryResult::ddlResult(const QString& message) {
    QueryResult res;
    res.m_type = Type::DDL;
    res.m_message = message;
    return res;
}

QueryResult QueryResult::infoResult(const QStringList& columns, const QVector<Row>& rows) {
    QueryResult res;
    res.m_type = Type::INFO;
    res.m_columns = columns;
    res.m_rows = rows;
    return res;
}

QueryResult QueryResult::error(const QString& message) {
    QueryResult res;
    res.m_success = false;
    res.m_errorMessage = message;
    return res;
}

} // namespace minidb
