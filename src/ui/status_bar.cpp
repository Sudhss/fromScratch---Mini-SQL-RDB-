#include "ui/status_bar.h"
#include "ui/theme.h"
#include <QHBoxLayout>

namespace minidb {

StatusBar::StatusBar(QWidget *parent) : QWidget(parent) {
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 2, 10, 2);
    
    dbPathLabel = new QLabel("No database open", this);
    queryStatusLabel = new QLabel("", this);
    rowCountLabel = new QLabel("", this);

    dbPathLabel->setStyleSheet(QString("color: %1;").arg(Theme::FG_MUTED));
    queryStatusLabel->setStyleSheet(QString("color: %1;").arg(Theme::FG_PRIMARY));
    rowCountLabel->setStyleSheet(QString("color: %1;").arg(Theme::FG_MUTED));
    
    queryStatusLabel->setAlignment(Qt::AlignCenter);
    rowCountLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    layout->addWidget(dbPathLabel, 1);
    layout->addWidget(queryStatusLabel, 1);
    layout->addWidget(rowCountLabel, 1);

    setStyleSheet(QString("background-color: %1;").arg(Theme::BG_TOOLBAR));
}

void StatusBar::setDatabasePath(const QString &path) {
    dbPathLabel->setText(path.isEmpty() ? "No database open" : path);
}

void StatusBar::setQueryStatus(const QString &status) {
    queryStatusLabel->setText(status);
}

void StatusBar::setRowCount(int count) {
    rowCountLabel->setText(count >= 0 ? QString("%1 rows returned").arg(count) : "");
}

void StatusBar::clearStatus() {
    queryStatusLabel->clear();
    rowCountLabel->clear();
}

} // namespace minidb
