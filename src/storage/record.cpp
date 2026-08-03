#include "storage/record.h"
#include <QtEndian>
#include <QDataStream>
#include <QIODevice>

namespace minidb {

QByteArray RecordSerializer::serialize(const Row& row, const TableSchema& schema) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    
    int numColumns = schema.columns.size();
    int numNullBytes = (numColumns + 7) / 8;
    QByteArray nullBitmap(numNullBytes, 0);
    
    for (int i = 0; i < numColumns; ++i) {
        if (row.size() <= i || row[i].isNull()) {
            nullBitmap[i / 8] = nullBitmap[i / 8] | (1 << (i % 8));
        }
    }
    
    stream.writeRawData(nullBitmap.constData(), numNullBytes);
    
    for (int i = 0; i < numColumns; ++i) {
        if (nullBitmap[i / 8] & (1 << (i % 8))) {
            continue; // Null, nothing to write
        }
        
        const auto& val = row[i];
        
        switch (schema.columns[i].type) {
            case DataType::INT:
                stream << static_cast<qint32>(val.toInt());
                break;
            case DataType::FLOAT:
                stream << val.toFloat();
                break;
            case DataType::VARCHAR: {
                QString str = val.toVarchar();
                QByteArray utf8 = str.toUtf8();
                stream << static_cast<quint16>(utf8.size());
                stream.writeRawData(utf8.constData(), utf8.size());
                break;
            }
            case DataType::BOOL:
                stream << static_cast<qint8>(val.toBool() ? 1 : 0);
                break;
            case DataType::DATE:
                stream << static_cast<qint32>(val.toInt()); // Date is stored as int
                break;
        }
    }
    
    return data;
}

Row RecordSerializer::deserialize(const QByteArray& data, const TableSchema& schema) {
    Row row;
    QDataStream stream(data);
    
    int numColumns = schema.columns.size();
    int numNullBytes = (numColumns + 7) / 8;
    
    QByteArray nullBitmap(numNullBytes, 0);
    stream.readRawData(nullBitmap.data(), numNullBytes);
    
    for (int i = 0; i < numColumns; ++i) {
        if (nullBitmap[i / 8] & (1 << (i % 8))) {
            row.push_back(Value());
            continue;
        }
        
        switch (schema.columns[i].type) {
            case DataType::INT: {
                qint32 val;
                stream >> val;
                row.push_back(Value(static_cast<int32_t>(val)));
                break;
            }
            case DataType::FLOAT: {
                double val;
                stream >> val;
                row.push_back(Value(val));
                break;
            }
            case DataType::VARCHAR: {
                quint16 len;
                stream >> len;
                QByteArray strData(len, 0);
                stream.readRawData(strData.data(), len);
                row.push_back(Value(QString::fromUtf8(strData)));
                break;
            }
            case DataType::BOOL: {
                qint8 val;
                stream >> val;
                row.push_back(Value(val != 0));
                break;
            }
            case DataType::DATE: {
                qint32 val;
                stream >> val;
                // Value has constructor for QDate, but we serialize it as int. We need to construct from QDate if that's what it expects, or we can just construct an int if the schema handles it.
                // Wait, Value(QDate) exists. How does QDate serialize? 
                row.push_back(Value(QDate::fromJulianDay(val)));
                break;
            }
        }
    }
    
    return row;
}

} // namespace minidb
