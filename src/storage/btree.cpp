#include "storage/btree.h"
#include <stdexcept>

namespace minidb {

BTree::BTree(Pager& pager, uint32_t rootPageId, DataType keyType)
    : m_pager(pager), m_rootPageId(rootPageId), m_keyType(keyType) {
    if (m_rootPageId == 0) {
        // Create root
        m_rootPageId = m_pager.allocatePage();
        Page p = Page::createEmpty(PageType::PG_INDEX_LEAF);
        m_pager.writePage(m_rootPageId, p);
    }
}

RowId BTree::search(const Value& key) {
    // Basic stub for v1
    (void)key;
    return {0, 0};
}

bool BTree::insert(const Value& key, RowId rid) {
    // Basic stub for v1
    (void)key;
    (void)rid;
    return true;
}

bool BTree::remove(const Value& key) {
    // Basic stub for v1
    (void)key;
    return true;
}

std::vector<std::pair<Value, RowId>> BTree::rangeScan(const Value& low, const Value& high) {
    // Basic stub for v1
    (void)low;
    (void)high;
    return {};
}

} // namespace minidb
