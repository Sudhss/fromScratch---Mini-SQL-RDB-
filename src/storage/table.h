#pragma once
#include "storage/pager.h"
#include "storage/record.h"
#include "core/types.h"

namespace minidb {

class TableIterator {
public:
    TableIterator(Pager& pager, uint32_t firstPageId, const TableSchema& schema);

    bool hasNext();
    std::pair<RowId, Row> next();
    void reset();

private:
    Pager& m_pager;
    uint32_t m_firstPageId;
    TableSchema m_schema;
    
    uint32_t m_currentPageId;
    int m_currentSlot;
    Page m_currentPage;

    void advanceToNextValidRecord();
};

class Table {
public:
    Table(Pager& pager, const TableSchema& schema, uint32_t firstPageId);

    RowId insertRow(const Row& row);
    Row getRow(RowId rid);
    bool deleteRow(RowId rid);
    bool updateRow(RowId rid, const Row& newRow);
    
    TableIterator scan();
    uint32_t rowCount() const;

private:
    Pager& m_pager;
    TableSchema m_schema;
    uint32_t m_firstPageId;
};

} // namespace minidb
