#include "ui/results_table.h"
#include "theme/theme.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QSortFilterProxyModel>

namespace minidb {

ResultsTableModel::ResultsTableModel(QObject *parent)
    : QAbstractTableModel(parent) {}

int ResultsTableModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(currentResult.rows.size());
}

int ResultsTableModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(currentResult.columns.size());
}

QVariant ResultsTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= currentResult.rows.size() || index.column() >= currentResult.columns.size())
        return QVariant();

    const auto &value = currentResult.rows[index.row()][index.column()];

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        if (std::holds_alternative<std::monostate>(value)) {
            return "NULL";
        } else if (std::holds_alternative<int>(value)) {
            return std::get<int>(value);
        } else if (std::holds_alternative<double>(value)) {
            return std::get<double>(value);
        } else if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value) ? "TRUE" : "FALSE";
        } else if (std::holds_alternative<QString>(value)) {
            return std::get<QString>(value);
        }
    } else if (role == Qt::ForegroundRole) {
        if (std::holds_alternative<std::monostate>(value)) {
            return QColor(Theme::FG_MUTED);
        }
    } else if (role == Qt::FontRole) {
        if (std::holds_alternative<std::monostate>(value)) {
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

    if (orientation == Qt::Horizontal && section < currentResult.columns.size()) {
        return currentResult.columns[section];
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
    if (result.type == QueryResult::Type::ERROR) {
        tableView->hide();
        statusLabel->setText(result.errorMessage);
        statusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::SYN_KEYWORD));
        statusLabel->show();
    } else if (result.type == QueryResult::Type::SELECT) {
        statusLabel->hide();
        model->setResult(result);
        tableView->resizeColumnsToContents();
        tableView->show();
    } else {
        tableView->hide();
        if (result.type == QueryResult::Type::MODIFICATION) {
            statusLabel->setText(QString("%1 rows affected.").arg(result.affectedRows));
        } else {
            statusLabel->setText("Query executed successfully.");
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
