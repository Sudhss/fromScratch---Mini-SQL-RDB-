#include "ui/schema_explorer.h"
#include "ui/theme.h"
#include <QVBoxLayout>
#include <QMenu>
#include <QHeaderView>

namespace minidb {

SchemaExplorer::SchemaExplorer(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    treeWidget = new QTreeWidget(this);
    treeWidget->setHeaderHidden(true);
    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    
    connect(treeWidget, &QTreeWidget::customContextMenuRequested, this, &SchemaExplorer::showContextMenu);
    connect(treeWidget, &QTreeWidget::itemDoubleClicked, [this](QTreeWidgetItem *item, int /* column */) {
        if (item && item->parent() && item->parent() == treeWidget->topLevelItem(0)) {
            emit tableSelected(item->text(0).remove("🗃️ ").trimmed());
        }
    });

    treeWidget->setStyleSheet(QString(
        "QTreeWidget {"
        "   background-color: %1;"
        "   color: %2;"
        "   border: none;"
        "}"
        "QTreeWidget::item:selected {"
        "   background-color: %3;"
        "}"
    ).arg(Theme::BG_SIDEBAR).arg(Theme::FG_PRIMARY).arg(Theme::BG_HOVER));

    layout->addWidget(treeWidget);
}

void SchemaExplorer::refresh(Catalog *catalog) {
    treeWidget->clear();
    
    if (!catalog) return;

    QTreeWidgetItem *rootItem = new QTreeWidgetItem(treeWidget);
    rootItem->setText(0, "📁 Database");
    rootItem->setFont(0, QFont("Segoe UI", 10, QFont::Bold));
    
    const auto &tables = catalog->listTables();
    for (const auto &tableName : tables) {
        QTreeWidgetItem *tableItem = new QTreeWidgetItem(rootItem);
        tableItem->setText(0, QString("🗃️ %1").arg(tableName));
        tableItem->setData(0, Qt::UserRole, tableName); // Store actual table name
        
        const auto &columns = catalog->getSchema(tableName).columns;
        for (const auto &col : columns) {
            QTreeWidgetItem *colItem = new QTreeWidgetItem(tableItem);
            
            QString colText = QString("📋 %1 (%2)").arg(col.name).arg(dataTypeToString(col.type));
            if (col.isPrimaryKey()) colText += " [PK]";
            if (col.isNotNull()) colText += " [NOT NULL]";
            
            colItem->setText(0, colText);
            colItem->setForeground(0, QColor(Theme::FG_MUTED));
        }
    }
    
    rootItem->setExpanded(true);
}

void SchemaExplorer::showContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = treeWidget->itemAt(pos);
    if (!item || !item->parent() || item->parent() != treeWidget->topLevelItem(0)) {
        return; // Only show menu for tables
    }

    QString tableName = item->data(0, Qt::UserRole).toString();

    QMenu menu(this);
    QAction *describeAction = menu.addAction("Describe Table");
    QAction *dropAction = menu.addAction("Drop Table");

    connect(describeAction, &QAction::triggered, [this, tableName]() {
        emit tableDescribeRequested(tableName);
    });
    
    connect(dropAction, &QAction::triggered, [this, tableName]() {
        emit tableDropRequested(tableName);
    });

    menu.setStyleSheet(QString(
        "QMenu { background-color: %1; color: %2; border: 1px solid %3; }"
        "QMenu::item:selected { background-color: %4; }"
    ).arg(Theme::BG_MENU).arg(Theme::FG_PRIMARY).arg(Theme::BORDER_COLOR).arg(Theme::BG_HOVER));

    menu.exec(treeWidget->viewport()->mapToGlobal(pos));
}

} // namespace minidb
