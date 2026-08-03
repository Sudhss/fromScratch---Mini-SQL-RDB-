#pragma once

#include <QWidget>
#include <QLabel>

namespace minidb {

class StatusBar : public QWidget {
    Q_OBJECT

public:
    explicit StatusBar(QWidget *parent = nullptr);

    void setDatabasePath(const QString &path);
    void setQueryStatus(const QString &status);
    void setRowCount(int count);
    void clearStatus();

private:
    QLabel *dbPathLabel;
    QLabel *queryStatusLabel;
    QLabel *rowCountLabel;
};

} // namespace minidb
