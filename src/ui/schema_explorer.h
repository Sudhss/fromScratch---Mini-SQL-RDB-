#pragma once

#include <QWidget>
#include <QTreeWidget>
#include "storage/catalog.h"

namespace minidb {

class SchemaExplorer : public QWidget {
    Q_OBJECT

public:
    explicit SchemaExplorer(QWidget *parent = nullptr);

    void refresh(Catalog *catalog);

signals:
    void tableSelected(const QString &tableName);
    void tableDropRequested(const QString &tableName);
    void tableDescribeRequested(const QString &tableName);

private slots:
    void showContextMenu(const QPoint &pos);

private:
    QTreeWidget *treeWidget;
};

} // namespace minidb
