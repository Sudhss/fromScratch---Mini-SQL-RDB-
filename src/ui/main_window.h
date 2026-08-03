#pragma once

#include <QMainWindow>
#include <QSplitter>
#include "core/database.h"
#include "ui/query_editor.h"
#include "ui/results_table.h"
#include "ui/schema_explorer.h"
#include "ui/status_bar.h"

namespace minidb {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void openDatabaseFile(const QString &path);

private slots:
    void newDatabase();
    void openDatabase();
    void executeQuery(const QString &sql = QString());
    void executeScript();
    void clearEditor();
    void clearResults();
    void about();
    void updateSchema();

private:
    void setupUi();
    void setupMenus();

    Database *db;
    
    QueryEditor *editor;
    ResultsTable *resultsTable;
    SchemaExplorer *schemaExplorer;
    StatusBar *statusBar;
    
    QSplitter *mainSplitter;
    QSplitter *rightSplitter;
};

} // namespace minidb
