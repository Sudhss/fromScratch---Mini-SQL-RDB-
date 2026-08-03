#pragma once
#include "storage/pager.h"
#include "core/types.h"
#include <QString>
#include <QHash>
#include <QStringList>

namespace minidb {

struct TableInfo {
    TableSchema schema;
    uint32_t firstPageId;
    uint32_t indexRootPageId;
};

class Catalog {
public:
    explicit Catalog(Pager& pager);

    uint32_t createTable(const TableSchema& schema);
    bool dropTable(const QString& name);
    TableSchema getSchema(const QString& name) const;
    bool hasTable(const QString& name) const;
    QStringList listTables() const;
    uint32_t getTablePageId(const QString& name) const;

    void load();
    void save();

private:
    Pager& m_pager;
    QHash<QString, TableInfo> m_tables;
};

} // namespace minidb
