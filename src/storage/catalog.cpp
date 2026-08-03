#include "storage/catalog.h"
#include <QDataStream>

namespace minidb {

Catalog::Catalog(Pager& pager) : m_pager(pager) {
}

uint32_t Catalog::createTable(const TableSchema& schema) {
    if (m_tables.contains(schema.name)) {
        return 0; // Already exists
    }
    
    uint32_t firstPageId = m_pager.allocatePage();
    Page p = Page::createEmpty(PageType::DATA);
    m_pager.writePage(firstPageId, p);
    
    TableInfo info{schema, firstPageId, 0};
    m_tables[schema.name] = info;
    
    save();
    return firstPageId;
}

bool Catalog::dropTable(const QString& name) {
    if (!m_tables.contains(name)) return false;
    
    m_tables.remove(name);
    save();
    return true;
}

TableSchema Catalog::getSchema(const QString& name) const {
    if (m_tables.contains(name)) {
        return m_tables[name].schema;
    }
    throw std::runtime_error("Table not found");
}

bool Catalog::hasTable(const QString& name) const {
    return m_tables.contains(name);
}

QStringList Catalog::listTables() const {
    return m_tables.keys();
}

uint32_t Catalog::getTablePageId(const QString& name) const {
    if (m_tables.contains(name)) {
        return m_tables[name].firstPageId;
    }
    return 0;
}

void Catalog::load() {
    FileHeader header = m_pager.getHeader();
    if (header.catalogPageId == 0) return;
    
    m_tables.clear();
    
    uint32_t currentPageId = header.catalogPageId;
    QByteArray fullData;
    
    while (currentPageId != 0) {
        Page p = m_pager.readPage(currentPageId);
        for (int i = 0; i < p.getRecordCount(); ++i) {
            fullData.append(p.getRecord(i));
        }
        currentPageId = p.getNextPageId();
    }
    
    if (fullData.isEmpty()) return;
    
    QDataStream stream(fullData);
    quint32 numTables;
    stream >> numTables;
    
    for (quint32 i = 0; i < numTables; ++i) {
        QString name;
        stream >> name;
        
        quint32 numColumns;
        stream >> numColumns;
        
        TableSchema schema;
        schema.name = name;
        
        for (quint32 j = 0; j < numColumns; ++j) {
            Column col;
            stream >> col.name;
            
            qint32 type;
            stream >> type;
            col.type = static_cast<DataType>(type);
            
            stream >> col.varchar_len;
            schema.columns.push_back(col);
        }
        
        quint32 firstPageId, indexRootPageId;
        stream >> firstPageId >> indexRootPageId;
        
        TableInfo info{schema, firstPageId, indexRootPageId};
        m_tables[name] = info;
    }
}

void Catalog::save() {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    
    stream << static_cast<quint32>(m_tables.size());
    
    for (auto it = m_tables.begin(); it != m_tables.end(); ++it) {
        stream << it.key();
        
        const TableInfo& info = it.value();
        stream << static_cast<quint32>(info.schema.columns.size());
        
        for (const auto& col : info.schema.columns) {
            stream << col.name;
            stream << static_cast<qint32>(col.type);
            stream << col.varchar_len;
        }
        
        stream << info.firstPageId;
        stream << info.indexRootPageId;
    }
    
    FileHeader header = m_pager.getHeader();
    if (header.catalogPageId == 0) {
        header.catalogPageId = m_pager.allocatePage();
        m_pager.updateHeader(header);
    }
    
    // Simplistic save: we assume catalog fits in a sequence of pages
    // Real implementation would free old pages and write new ones
    uint32_t currentPageId = header.catalogPageId;
    Page p = Page::createEmpty(PageType::CATALOG);
    
    int offset = 0;
    while (offset < data.size()) {
        int chunkSize = std::min<int>(data.size() - offset, PAGE_SIZE - PAGE_HEADER_SIZE - SLOT_SIZE);
        p.addRecord(data.mid(offset, chunkSize));
        offset += chunkSize;
        
        if (offset < data.size()) {
            uint32_t nextPageId = m_pager.allocatePage();
            p.setNextPageId(nextPageId);
            m_pager.writePage(currentPageId, p);
            
            currentPageId = nextPageId;
            p = Page::createEmpty(PageType::CATALOG);
        }
    }
    
    m_pager.writePage(currentPageId, p);
}

} // namespace minidb
