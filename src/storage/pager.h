#pragma once
#include "storage/page.h"
#include <QString>
#include <QFile>
#include <QHash>
#include <list>
#include <memory>

namespace minidb {

struct CacheEntry {
    Page page;
    bool dirty;
    uint32_t pageId;
};

#pragma pack(push, 1)
struct FileHeader {
    char magic[4]; // 'MNDB'
    uint16_t version;
    uint32_t pageCount;
    uint32_t firstFreePage;
    uint32_t catalogPageId;
};
#pragma pack(pop)

class Pager {
public:
    explicit Pager(const QString& filePath, size_t maxCacheSize = 256);
    ~Pager();

    Page readPage(uint32_t pageId);
    void writePage(uint32_t pageId, const Page& page);
    uint32_t allocatePage();
    void flushAll();
    void flush(uint32_t pageId);
    uint32_t pageCount() const;
    void close();

    FileHeader getHeader() const { return m_header; }
    void updateHeader(const FileHeader& header);

private:
    void initFile();
    void evictOne();

    QString m_filePath;
    QFile m_file;
    size_t m_maxCacheSize;
    FileHeader m_header;

    QHash<uint32_t, std::list<CacheEntry>::iterator> m_cacheMap;
    std::list<CacheEntry> m_cacheList;
};

} // namespace minidb
