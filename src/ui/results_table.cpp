#include "ui/results_table.h"
#include "ui/theme.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QSortFilterProxyModel>

namespace minidb {

ResultsTableModel::ResultsTableModel(QObject *parent)
    : QAbstractTableModel(parent) {}

int ResultsTableModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return currentResult.rowCount();
}

int ResultsTableModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return currentResult.columnCount();
}

QVariant ResultsTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= currentResult.rowCount() || index.column() >= currentResult.columnCount())
        return QVariant();

    Value value = currentResult.rows()[index.row()][index.column()];

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        if (value.isNull()) {
            return "NULL";
        } else {
            return value.toString();
        }
    } else if (role == Qt::ForegroundRole) {
        if (value.isNull()) {
            return QColor(Theme::FG_MUTED);
        }
    } else if (role == Qt::FontRole) {
        if (value.isNull()) {
            QFont font;
            font.setItalic(true);
            return font;
        }
    }
    return QVariant();
}

QVariant ResultsTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal && section < currentResult.columnCount()) {
        return currentResult.columns()[section];
    } else if (orientation == Qt::Vertical) {
        return section + 1;
    }
    return QVariant();
}

void ResultsTableModel::setResult(const QueryResult &result) {
    beginResetModel();
    currentResult = result;
    endResetModel();
}

void ResultsTableModel::clear() {
    beginResetModel();
    currentResult = QueryResult{};
    endResetModel();
}


ResultsTable::ResultsTable(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    tableView = new QTableView(this);
    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setMargin(10);
    statusLabel->hide();

    model = new ResultsTableModel(this);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    tableView->setModel(proxyModel);

    tableView->setSortingEnabled(true);
    tableView->setAlternatingRowColors(true);
    tableView->horizontalHeader()->setStretchLastSection(true);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Theme setup
    tableView->setStyleSheet(QString(
        "QTableView {"
        "   background-color: %1;"
        "   alternate-background-color: %2;"
        "   color: %3;"
        "   gridline-color: %4;"
        "   border: none;"
        "}"
        "QHeaderView::section {"
        "   background-color: %2;"
        "   color: %3;"
        "   padding: 4px;"
        "   border: 1px solid %4;"
        "}"
    ).arg(Theme::BG_MAIN).arg(Theme::BG_SIDEBAR).arg(Theme::FG_PRIMARY).arg(Theme::BORDER_COLOR));
    
    layout->addWidget(statusLabel);
    layout->addWidget(tableView);
}

void ResultsTable::setResult(const QueryResult &result) {
    if (result.hasError()) {
        tableView->hide();
        statusLabel->setText(result.errorMessage());
        statusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::SYN_KEYWORD));
        statusLabel->show();
    } else if (result.type() == QueryResult::Type::SELECT || result.type() == QueryResult::Type::INFO) {
        statusLabel->hide();
        model->setResult(result);
        tableView->resizeColumnsToContents();
        tableView->show();
    } else {
        tableView->hide();
        if (result.type() == QueryResult::Type::MODIFICATION) {
            statusLabel->setText(QString("%1 rows affected.").arg(result.affectedRows()));
        } else {
            statusLabel->setText(result.message().isEmpty() ? "Query executed successfully." : result.message());
        }
        statusLabel->setStyleSheet(QString("color: %1;").arg(Theme::FG_PRIMARY));
        statusLabel->show();
    }
}

void ResultsTable::clear() {
    model->clear();
    statusLabel->clear();
    statusLabel->hide();
    tableView->show();
}

} // namespace minidb
