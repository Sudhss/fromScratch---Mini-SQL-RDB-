#pragma once

#include <QAbstractTableModel>
#include <QWidget>
#include <QTableView>
#include <QLabel>
#include "sql/result.h"

namespace minidb {

class ResultsTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit ResultsTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setResult(const QueryResult &result);
    void clear();

private:
    QueryResult currentResult;
};

class ResultsTable : public QWidget {
    Q_OBJECT

public:
    explicit ResultsTable(QWidget *parent = nullptr);

    void setResult(const QueryResult &result);
    void clear();

private:
    QTableView *tableView;
    QLabel *statusLabel;
    ResultsTableModel *model;
};

} // namespace minidb
