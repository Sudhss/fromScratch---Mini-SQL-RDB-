#pragma once
#include "storage/pager.h"
#include "core/types.h"
#include <vector>

namespace minidb {

class BTree {
public:
    BTree(Pager& pager, uint32_t rootPageId, DataType keyType);

    RowId search(const Value& key);
    bool insert(const Value& key, RowId rid);
    bool remove(const Value& key);
    std::vector<std::pair<Value, RowId>> rangeScan(const Value& low, const Value& high);

private:
    Pager& m_pager;
    uint32_t m_rootPageId;
    DataType m_keyType;

    // Helper methods for traversal, splitting, etc. would go here
};

} // namespace minidb
