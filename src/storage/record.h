#pragma once
#include <QByteArray>
#include "core/types.h"

namespace minidb {

class RecordSerializer {
public:
    static QByteArray serialize(const Row& row, const TableSchema& schema);
    static Row deserialize(const QByteArray& data, const TableSchema& schema);
};

} // namespace minidb
