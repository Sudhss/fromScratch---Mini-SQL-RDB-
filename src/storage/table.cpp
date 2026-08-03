#include "storage/table.h"

namespace minidb {

TableIterator::TableIterator(Pager& pager, uint32_t firstPageId, const TableSchema& schema)
    : m_pager(pager), m_firstPageId(firstPageId), m_schema(schema),
      m_currentPageId(firstPageId), m_currentSlot(-1) {
    if (m_currentPageId != 0) {
        m_currentPage = m_pager.readPage(m_currentPageId);
        advanceToNextValidRecord();
    }
}

void TableIterator::advanceToNextValidRecord() {
    while (m_currentPageId != 0) {
        m_currentSlot++;
        if (m_currentSlot < m_currentPage.getRecordCount()) {
            QByteArray record = m_currentPage.getRecord(m_currentSlot);
            if (!record.isEmpty()) {
                return; // Found next valid record
            }
        } else {
            // Move to next page
            m_currentPageId = m_currentPage.getNextPageId();
            if (m_currentPageId != 0) {
                m_currentPage = m_pager.readPage(m_currentPageId);
                m_currentSlot = -1;
            }
        }
    }
}

bool TableIterator::hasNext() {
    return m_currentPageId != 0;
}

std::pair<RowId, Row> TableIterator::next() {
    if (!hasNext()) return {};
    
    QByteArray record = m_currentPage.getRecord(m_currentSlot);
    Row row = RecordSerializer::deserialize(record, m_schema);
    RowId rid = { m_currentPageId, static_cast<uint16_t>(m_currentSlot) };
    
    advanceToNextValidRecord();
    
    return { rid, row };
}

void TableIterator::reset() {
    m_currentPageId = m_firstPageId;
    m_currentSlot = -1;
    if (m_currentPageId != 0) {
        m_currentPage = m_pager.readPage(m_currentPageId);
        advanceToNextValidRecord();
    }
}

Table::Table(Pager& pager, const TableSchema& schema, uint32_t firstPageId)
    : m_pager(pager), m_schema(schema), m_firstPageId(firstPageId) {
}

RowId Table::insertRow(const Row& row) {
    QByteArray data = RecordSerializer::serialize(row, m_schema);
    
    uint32_t currentPageId = m_firstPageId;
    uint32_t lastPageId = 0;
    
    while (currentPageId != 0) {
        Page p = m_pager.readPage(currentPageId);
        int slot = p.addRecord(data);
        if (slot != -1) {
            m_pager.writePage(currentPageId, p);
            return { currentPageId, static_cast<uint16_t>(slot) };
        }
        lastPageId = currentPageId;
        currentPageId = p.getNextPageId();
    }
    
    // Allocate new page
    uint32_t newPageId = m_pager.allocatePage();
    Page newPage = Page::createEmpty(PageType::DATA);
    int slot = newPage.addRecord(data);
    m_pager.writePage(newPageId, newPage);
    
    if (lastPageId != 0) {
        Page lastPage = m_pager.readPage(lastPageId);
        lastPage.setNextPageId(newPageId);
        m_pager.writePage(lastPageId, lastPage);
    } else {
        m_firstPageId = newPageId;
    }
    
    return { newPageId, static_cast<uint16_t>(slot) };
}

Row Table::getRow(RowId rid) {
    Page p = m_pager.readPage(rid.pageId);
    QByteArray data = p.getRecord(rid.slotId);
    if (data.isEmpty()) return {};
    return RecordSerializer::deserialize(data, m_schema);
}

bool Table::deleteRow(RowId rid) {
    Page p = m_pager.readPage(rid.pageId);
    bool ok = p.deleteRecord(rid.slotId);
    if (ok) {
        m_pager.writePage(rid.pageId, p);
    }
    return ok;
}

bool Table::updateRow(RowId rid, const Row& newRow) {
    QByteArray data = RecordSerializer::serialize(newRow, m_schema);
    Page p = m_pager.readPage(rid.pageId);
    bool ok = p.updateRecord(rid.slotId, data);
    if (ok) {
        m_pager.writePage(rid.pageId, p);
    } else {
        // If it doesn't fit, we'd need to delete and re-insert (giving a new RowId).
        // For simplicity in a heap file, if update in place fails, we might return false.
        // A full implementation would handle forwarding pointers or throw an error indicating RowId changed.
    }
    return ok;
}

TableIterator Table::scan() {
    return TableIterator(m_pager, m_firstPageId, m_schema);
}

uint32_t Table::rowCount() const {
    uint32_t count = 0;
    uint32_t currentPageId = m_firstPageId;
    while (currentPageId != 0) {
        Page p = m_pager.readPage(currentPageId);
        for (int i = 0; i < p.getRecordCount(); ++i) {
            if (!p.getRecord(i).isEmpty()) {
                count++;
            }
        }
        currentPageId = p.getNextPageId();
    }
    return count;
}

} // namespace minidb
