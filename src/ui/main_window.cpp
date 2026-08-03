#include "ui/main_window.h"
#include "theme/theme.h"
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QElapsedTimer>
#include <QTimer>

namespace minidb {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), db(nullptr) {
    setupUi();
    setupMenus();
    
    setWindowTitle("MiniSQL RDB");
    resize(1200, 800);
    
    setStyleSheet(Theme::darkStyleSheet());
    
    // Connect schema explorer actions
    connect(schemaExplorer, &SchemaExplorer::tableDescribeRequested, [this](const QString &tableName) {
        editor->appendPlainText(QString("\nDESCRIBE %1;").arg(tableName));
    });
    
    connect(schemaExplorer, &SchemaExplorer::tableDropRequested, [this](const QString &tableName) {
        editor->appendPlainText(QString("\nDROP TABLE %1;").arg(tableName));
    });
}

MainWindow::~MainWindow() {
    delete db;
}

void MainWindow::setupUi() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainSplitter = new QSplitter(Qt::Horizontal, this);
    rightSplitter = new QSplitter(Qt::Vertical, this);

    schemaExplorer = new SchemaExplorer(this);
    editor = new QueryEditor(this);
    resultsTable = new ResultsTable(this);
    statusBar = new StatusBar(this);

    rightSplitter->addWidget(editor);
    rightSplitter->addWidget(resultsTable);
    rightSplitter->setSizes({static_cast<int>(height() * 0.4), static_cast<int>(height() * 0.6)});

    mainSplitter->addWidget(schemaExplorer);
    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setSizes({static_cast<int>(width() * 0.25), static_cast<int>(width() * 0.75)});

    mainLayout->addWidget(mainSplitter, 1);
    mainLayout->addWidget(statusBar, 0);

    connect(editor, &QueryEditor::executeRequested, this, [this](const QString &sql) { executeQuery(sql); });
    connect(editor, &QueryEditor::executeSelectedRequested, this, [this](const QString &sql) { executeQuery(sql); });
}

void MainWindow::setupMenus() {
    QMenuBar *menuBar = this->menuBar();
    menuBar->setFont(QFont("Segoe UI", 10));

    // File Menu
    QMenu *fileMenu = menuBar->addMenu("File");
    QAction *newDbAction = fileMenu->addAction("New Database...");
    newDbAction->setShortcut(QKeySequence::New);
    connect(newDbAction, &QAction::triggered, this, &MainWindow::newDatabase);

    QAction *openDbAction = fileMenu->addAction("Open Database...");
    openDbAction->setShortcut(QKeySequence::Open);
    connect(openDbAction, &QAction::triggered, this, &MainWindow::openDatabase);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction("Exit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    // Query Menu
    QMenu *queryMenu = menuBar->addMenu("Query");
    QAction *execAction = queryMenu->addAction("Execute");
    execAction->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(execAction, &QAction::triggered, [this]() { executeQuery(); });

    queryMenu->addSeparator();
    connect(queryMenu->addAction("Clear Editor"), &QAction::triggered, this, &MainWindow::clearEditor);
    connect(queryMenu->addAction("Clear Results"), &QAction::triggered, this, &MainWindow::clearResults);

    // Help Menu
    QMenu *helpMenu = menuBar->addMenu("Help");
    connect(helpMenu->addAction("About"), &QAction::triggered, this, &MainWindow::about);

    // ToolBar
    QToolBar *toolbar = addToolBar("Main");
    toolbar->setMovable(false);
    toolbar->addAction(newDbAction);
    toolbar->addAction(openDbAction);
    toolbar->addSeparator();
    
    QAction *runAction = toolbar->addAction("▶ Execute");
    runAction->setToolTip("Execute Query (Ctrl+Enter)");
    connect(runAction, &QAction::triggered, [this]() { executeQuery(); });
    
    toolbar->addAction("Clear", this, &MainWindow::clearEditor);
}

void MainWindow::newDatabase() {
    QString path = QFileDialog::getSaveFileName(this, "New Database", QString(), "MiniDB Files (*.minidb);;All Files (*)");
    if (!path.isEmpty()) {
        openDatabaseFile(path);
    }
}

void MainWindow::openDatabase() {
    QString path = QFileDialog::getOpenFileName(this, "Open Database", QString(), "MiniDB Files (*.minidb);;All Files (*)");
    if (!path.isEmpty()) {
        openDatabaseFile(path);
    }
}

void MainWindow::openDatabaseFile(const QString &path) {
    if (db) {
        delete db;
        db = nullptr;
    }
    
    try {
        db = new Database(path);
        statusBar->setDatabasePath(path);
        updateSchema();
        statusBar->setQueryStatus("Database opened successfully.");
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Error", QString("Failed to open database: %1").arg(e.what()));
        statusBar->setDatabasePath("");
    }
}

void MainWindow::executeQuery(const QString &sql) {
    if (!db) {
        QMessageBox::warning(this, "No Database", "Please open or create a database first.");
        return;
    }

    QString query = sql;
    if (query.isEmpty()) {
        query = editor->toPlainText();
    }
    
    if (query.trimmed().isEmpty()) return;

    QElapsedTimer timer;
    timer.start();

    try {
        QueryResult result = db->execute(query);
        qint64 elapsed = timer.elapsed();

        resultsTable->setResult(result);
        
        statusBar->setQueryStatus(QString("Query executed in %1 ms").arg(elapsed));
        
        if (result.type == QueryResult::Type::SELECT) {
            statusBar->setRowCount(static_cast<int>(result.rows.size()));
        } else {
            statusBar->setRowCount(-1);
            if (result.type == QueryResult::Type::DDL) {
                updateSchema(); // Refresh schema tree on DDL statements
            }
        }
    } catch (const std::exception &e) {
        QueryResult errResult;
        errResult.type = QueryResult::Type::ERROR;
        errResult.errorMessage = e.what();
        resultsTable->setResult(errResult);
        statusBar->setQueryStatus("Error executing query.");
        statusBar->setRowCount(-1);
    }
}

void MainWindow::clearEditor() {
    editor->clear();
}

void MainWindow::clearResults() {
    resultsTable->clear();
    statusBar->clearStatus();
}

void MainWindow::updateSchema() {
    if (db) {
        schemaExplorer->refresh(db->getCatalog());
    } else {
        schemaExplorer->refresh(nullptr);
    }
}

void MainWindow::about() {
    QMessageBox::about(this, "About MiniSQL RDB",
        "<b>MiniSQL Relational Database</b><br><br>"
        "A from-scratch SQL relational database management system built with C++ and Qt6.<br>"
        "Features custom storage engine, SQL parser, query execution, and a dedicated UI.");
}

} // namespace minidb
