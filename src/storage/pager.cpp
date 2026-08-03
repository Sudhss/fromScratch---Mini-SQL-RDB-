#include "storage/pager.h"
#include <QFileInfo>
#include <stdexcept>
#include <cstring>

namespace minidb {

Pager::Pager(const QString& filePath, size_t maxCacheSize)
    : m_filePath(filePath), m_file(filePath), m_maxCacheSize(maxCacheSize) {
    
    if (!m_file.open(QIODevice::ReadWrite)) {
        throw std::runtime_error("Could not open database file");
    }
    
    if (m_file.size() == 0) {
        initFile();
    } else {
        m_file.seek(0);
        m_file.read(reinterpret_cast<char*>(&m_header), sizeof(FileHeader));
    }
}

Pager::~Pager() {
    close();
}

void Pager::initFile() {
    memset(&m_header, 0, sizeof(FileHeader));
    memcpy(m_header.magic, "MNDB", 4);
    m_header.version = 1;
    m_header.pageCount = 1; // page 0 is header
    m_header.firstFreePage = 0;
    m_header.catalogPageId = 0;
    
    Page headerPage;
    memcpy(headerPage.data().data(), &m_header, sizeof(FileHeader));
    m_file.seek(0);
    m_file.write(headerPage.rawData(), PAGE_SIZE);
}

Page Pager::readPage(uint32_t pageId) {
    if (m_cacheMap.contains(pageId)) {
        auto it = m_cacheMap[pageId];
        m_cacheList.splice(m_cacheList.begin(), m_cacheList, it); // Move to front
        return it->page;
    }
    
    if (pageId >= m_header.pageCount) {
        throw std::runtime_error("Read past end of file");
    }
    
    m_file.seek(pageId * PAGE_SIZE);
    QByteArray data = m_file.read(PAGE_SIZE);
    
    if (data.size() < PAGE_SIZE) {
        data.append(PAGE_SIZE - data.size(), '\0');
    }
    
    Page p(data);
    
    if (m_cacheMap.size() >= m_maxCacheSize) {
        evictOne();
    }
    
    m_cacheList.push_front({p, false, pageId});
    m_cacheMap[pageId] = m_cacheList.begin();
    
    return p;
}

void Pager::writePage(uint32_t pageId, const Page& page) {
    if (m_cacheMap.contains(pageId)) {
        auto it = m_cacheMap[pageId];
        it->page = page;
        it->dirty = true;
        m_cacheList.splice(m_cacheList.begin(), m_cacheList, it);
    } else {
        if (m_cacheMap.size() >= m_maxCacheSize) {
            evictOne();
        }
        m_cacheList.push_front({page, true, pageId});
        m_cacheMap[pageId] = m_cacheList.begin();
    }
}

uint32_t Pager::allocatePage() {
    uint32_t newPageId = m_header.pageCount++;
    m_file.resize(m_header.pageCount * PAGE_SIZE);
    updateHeader(m_header);
    
    Page p = Page::createEmpty(PageType::PG_FREE);
    writePage(newPageId, p);
    
    return newPageId;
}

void Pager::evictOne() {
    if (m_cacheList.empty()) return;
    auto last = std::prev(m_cacheList.end());
    if (last->dirty) {
        flush(last->pageId);
    }
    m_cacheMap.remove(last->pageId);
    m_cacheList.erase(last);
}

void Pager::flush(uint32_t pageId) {
    if (!m_cacheMap.contains(pageId)) return;
    auto it = m_cacheMap[pageId];
    if (it->dirty) {
        m_file.seek(pageId * PAGE_SIZE);
        m_file.write(it->page.rawData(), PAGE_SIZE);
        it->dirty = false;
    }
}

void Pager::flushAll() {
    for (auto& entry : m_cacheList) {
        if (entry.dirty) {
            m_file.seek(entry.pageId * PAGE_SIZE);
            m_file.write(entry.page.rawData(), PAGE_SIZE);
            entry.dirty = false;
        }
    }
    m_file.flush();
}

uint32_t Pager::pageCount() const {
    return m_header.pageCount;
}

void Pager::close() {
    if (m_file.isOpen()) {
        flushAll();
        m_file.close();
    }
}

void Pager::updateHeader(const FileHeader& header) {
    m_header = header;
    m_file.seek(0);
    m_file.write(reinterpret_cast<const char*>(&m_header), sizeof(FileHeader));
}

} // namespace minidb
