#pragma once
#include <cstdint>
#include <QByteArray>

namespace minidb {

constexpr uint32_t PAGE_SIZE = 4096;
constexpr uint32_t PAGE_HEADER_SIZE = 16;
constexpr uint32_t SLOT_SIZE = 4;  // 2 bytes offset + 2 bytes length

enum class PageType : uint8_t { 
    PG_FREE = 0, 
    PG_DATA = 1, 
    PG_INDEX_INTERNAL = 2, 
    PG_INDEX_LEAF = 3, 
    PG_CATALOG = 4, 
    PG_OVERFLOW = 5 
};

class Page {
public:
    Page();
    explicit Page(const QByteArray& data);

    static Page createEmpty(PageType type);

    PageType getPageType() const;
    void setPageType(PageType type);

    uint16_t getRecordCount() const;
    void setRecordCount(uint16_t count);

    uint16_t getFreeSpaceStart() const;
    void setFreeSpaceStart(uint16_t start);

    uint16_t getFreeSpaceEnd() const;
    void setFreeSpaceEnd(uint16_t end);

    uint32_t getNextPageId() const;
    void setNextPageId(uint32_t pageId);

    int addRecord(const QByteArray& data);
    QByteArray getRecord(int slotIndex) const;
    bool deleteRecord(int slotIndex);
    bool updateRecord(int slotIndex, const QByteArray& data);

    uint16_t freeSpace() const;
    void clear();

    const char* rawData() const;
    QByteArray& data() { return m_data; }

private:
    QByteArray m_data;

    void updateHeader(uint16_t recordCount, uint16_t freeStart, uint16_t freeEnd);
};

} // namespace minidb
