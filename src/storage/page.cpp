#include "storage/page.h"
#include <cstring>
#include <QtEndian>

namespace minidb {

Page::Page() : m_data(PAGE_SIZE, '\0') {
    clear();
}

Page::Page(const QByteArray& data) {
    if (data.size() == PAGE_SIZE) {
        m_data = data;
    } else {
        m_data = QByteArray(PAGE_SIZE, '\0');
    }
}

Page Page::createEmpty(PageType type) {
    Page p;
    p.setPageType(type);
    return p;
}

PageType Page::getPageType() const {
    return static_cast<PageType>(m_data.at(0));
}

void Page::setPageType(PageType type) {
    m_data[0] = static_cast<char>(type);
}

uint16_t Page::getRecordCount() const {
    return qFromBigEndian<uint16_t>(m_data.constData() + 1);
}

void Page::setRecordCount(uint16_t count) {
    qToBigEndian<uint16_t>(count, m_data.data() + 1);
}

uint16_t Page::getFreeSpaceStart() const {
    return qFromBigEndian<uint16_t>(m_data.constData() + 3);
}

void Page::setFreeSpaceStart(uint16_t start) {
    qToBigEndian<uint16_t>(start, m_data.data() + 3);
}

uint16_t Page::getFreeSpaceEnd() const {
    return qFromBigEndian<uint16_t>(m_data.constData() + 5);
}

void Page::setFreeSpaceEnd(uint16_t end) {
    qToBigEndian<uint16_t>(end, m_data.data() + 5);
}

uint32_t Page::getNextPageId() const {
    return qFromBigEndian<uint32_t>(m_data.constData() + 7);
}

void Page::setNextPageId(uint32_t pageId) {
    qToBigEndian<uint32_t>(pageId, m_data.data() + 7);
}

int Page::addRecord(const QByteArray& data) {
    uint16_t size = data.size();
    if (freeSpace() < size + SLOT_SIZE) {
        return -1;
    }
    
    uint16_t count = getRecordCount();
    uint16_t freeStart = getFreeSpaceStart();
    uint16_t freeEnd = getFreeSpaceEnd();
    
    int slotIndex = -1;
    // Check for deleted slots first
    for (int i = 0; i < count; ++i) {
        uint16_t offset = qFromBigEndian<uint16_t>(m_data.constData() + PAGE_HEADER_SIZE + i * SLOT_SIZE);
        uint16_t len = qFromBigEndian<uint16_t>(m_data.constData() + PAGE_HEADER_SIZE + i * SLOT_SIZE + 2);
        if (offset == 0 && len == 0) {
            slotIndex = i;
            break;
        }
    }
    
    if (slotIndex == -1) {
        slotIndex = count;
        count++;
        freeStart += SLOT_SIZE;
    }
    
    freeEnd -= size;
    
    // Write data
    memcpy(m_data.data() + freeEnd, data.constData(), size);
    
    // Write slot
    qToBigEndian<uint16_t>(freeEnd, m_data.data() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE);
    qToBigEndian<uint16_t>(size, m_data.data() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE + 2);
    
    updateHeader(count, freeStart, freeEnd);
    return slotIndex;
}

QByteArray Page::getRecord(int slotIndex) const {
    if (slotIndex < 0 || slotIndex >= getRecordCount()) return {};
    
    uint16_t offset = qFromBigEndian<uint16_t>(m_data.constData() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE);
    uint16_t len = qFromBigEndian<uint16_t>(m_data.constData() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE + 2);
    
    if (offset == 0 && len == 0) return {}; // deleted
    
    return m_data.mid(offset, len);
}

bool Page::deleteRecord(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= getRecordCount()) return false;
    
    qToBigEndian<uint16_t>(0, m_data.data() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE);
    qToBigEndian<uint16_t>(0, m_data.data() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE + 2);
    return true;
}

bool Page::updateRecord(int slotIndex, const QByteArray& data) {
    if (slotIndex < 0 || slotIndex >= getRecordCount()) return false;
    
    uint16_t offset = qFromBigEndian<uint16_t>(m_data.constData() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE);
    uint16_t len = qFromBigEndian<uint16_t>(m_data.constData() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE + 2);
    
    if (offset == 0 && len == 0) return false;
    
    if (data.size() <= len) {
        // In-place update
        memcpy(m_data.data() + offset, data.constData(), data.size());
        qToBigEndian<uint16_t>(data.size(), m_data.data() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE + 2);
        return true;
    }
    
    // Delete and re-add if size changed and new size is larger
    // A proper update would reuse space or compact page. For now, simple delete + re-insert in a new slot is not an option since we must preserve slotIndex.
    // Instead we just free the old space (mark deleted) and allocate from free space, updating the slot.
    if (freeSpace() < data.size()) return false;
    
    uint16_t freeEnd = getFreeSpaceEnd() - data.size();
    memcpy(m_data.data() + freeEnd, data.constData(), data.size());
    
    qToBigEndian<uint16_t>(freeEnd, m_data.data() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE);
    qToBigEndian<uint16_t>(data.size(), m_data.data() + PAGE_HEADER_SIZE + slotIndex * SLOT_SIZE + 2);
    
    setFreeSpaceEnd(freeEnd);
    return true;
}

uint16_t Page::freeSpace() const {
    return getFreeSpaceEnd() - getFreeSpaceStart();
}

void Page::clear() {
    m_data.fill('\0');
    setPageType(PageType::PG_FREE);
    updateHeader(0, PAGE_HEADER_SIZE, PAGE_SIZE);
    setNextPageId(0);
}

const char* Page::rawData() const {
    return m_data.constData();
}

void Page::updateHeader(uint16_t recordCount, uint16_t freeStart, uint16_t freeEnd) {
    setRecordCount(recordCount);
    setFreeSpaceStart(freeStart);
    setFreeSpaceEnd(freeEnd);
}

} // namespace minidb
