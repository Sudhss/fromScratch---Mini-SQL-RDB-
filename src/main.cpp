#include <QApplication>
#include <QCommandLineParser>
#include <QTextStream>
#include <QFile>

#include "core/database.h"
#include "ui/main_window.h"
#include "theme/theme.h"

// ── CLI REPL ──────────────────────────────────────────────────────────

int runCli(const QString& dbPath) {
    QTextStream out(stdout);
    QTextStream in(stdin);

    minidb::Database db;

    if (dbPath.isEmpty()) {
        out << "MiniSQL RDB — Interactive CLI\n";
        out << "Usage: .open <file>  | .create <file> | .quit\n\n";
    } else {
        QFile f(dbPath);
        if (f.exists()) {
            if (!db.open(dbPath)) {
                out << "Error: Could not open database '" << dbPath << "'\n";
                return 1;
            }
            out << "Opened database: " << dbPath << "\n";
        } else {
            if (!db.create(dbPath)) {
                out << "Error: Could not create database '" << dbPath << "'\n";
                return 1;
            }
            out << "Created database: " << dbPath << "\n";
        }
    }

    out << "MiniSQL> " << Qt::flush;

    QString lineBuffer;
    while (!in.atEnd()) {
        QString line = in.readLine();

        // Meta-commands (dot commands)
        if (line.trimmed().startsWith('.')) {
            QString cmd = line.trimmed();
            if (cmd == ".quit" || cmd == ".exit") {
                break;
            } else if (cmd.startsWith(".open ")) {
                QString path = cmd.mid(6).trimmed();
                if (db.open(path))
                    out << "Opened: " << path << "\n";
                else
                    out << "Error: Could not open '" << path << "'\n";
            } else if (cmd.startsWith(".create ")) {
                QString path = cmd.mid(8).trimmed();
                if (db.create(path))
                    out << "Created: " << path << "\n";
                else
                    out << "Error: Could not create '" << path << "'\n";
            } else if (cmd == ".tables") {
                auto result = db.execute("SHOW TABLES;");
                if (result.hasError()) {
                    out << result.errorMessage() << "\n";
                } else {
                    for (int i = 0; i < result.rowCount(); ++i) {
                        const auto& row = result.rows()[i];
                        if (!row.empty())
                            out << row[0].toString() << "\n";
                    }
                }
            } else if (cmd == ".help") {
                out << "Commands:\n";
                out << "  .open <file>     Open an existing database\n";
                out << "  .create <file>   Create a new database\n";
                out << "  .tables          Show all tables\n";
                out << "  .quit / .exit    Exit the CLI\n";
                out << "  .help            Show this help\n";
                out << "\nOr type any SQL statement (end with ;)\n";
            } else {
                out << "Unknown command. Type .help for help.\n";
            }
            out << "MiniSQL> " << Qt::flush;
            continue;
        }

        // Accumulate multi-line SQL (wait for semicolon)
        lineBuffer += line + " ";
        if (!lineBuffer.trimmed().endsWith(';')) {
            out << "   ...> " << Qt::flush;
            continue;
        }

        // Execute SQL
        QString sql = lineBuffer.trimmed();
        lineBuffer.clear();

        if (sql.isEmpty()) {
            out << "MiniSQL> " << Qt::flush;
            continue;
        }

        if (!db.isOpen()) {
            out << "Error: No database open. Use .open or .create first.\n";
            out << "MiniSQL> " << Qt::flush;
            continue;
        }

        auto result = db.execute(sql);

        if (result.hasError()) {
            out << "Error: " << result.errorMessage() << "\n";
        } else if (result.type() == minidb::QueryResult::Type::SELECT
                || result.type() == minidb::QueryResult::Type::INFO) {
            // Print column headers
            const auto& cols = result.columns();
            for (int i = 0; i < cols.size(); ++i) {
                if (i > 0) out << " | ";
                out << cols[i].leftJustified(15);
            }
            out << "\n";

            // Separator line
            for (int i = 0; i < cols.size(); ++i) {
                if (i > 0) out << "-+-";
                out << QString(15, '-');
            }
            out << "\n";

            // Print rows
            const auto& rows = result.rows();
            for (const auto& row : rows) {
                for (int i = 0; i < static_cast<int>(row.size()); ++i) {
                    if (i > 0) out << " | ";
                    out << row[i].toString().leftJustified(15);
                }
                out << "\n";
            }
            out << "(" << result.rowCount() << " rows)\n";
        } else if (result.type() == minidb::QueryResult::Type::MODIFICATION) {
            out << result.message() << "\n";
        } else {
            out << result.message() << "\n";
        }

        out << "MiniSQL> " << Qt::flush;
    }

    out << "\nBye!\n";
    return 0;
}

// ── Main Entry Point ──────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("MiniSQL RDB");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("FromScratch");

    QCommandLineParser cmdParser;
    cmdParser.setApplicationDescription("Mini SQL Relational Database — Built from Scratch");
    cmdParser.addHelpOption();
    cmdParser.addVersionOption();

    QCommandLineOption cliOption(
        QStringList() << "c" << "cli",
        "Run in CLI (command-line) mode instead of GUI."
    );
    cmdParser.addOption(cliOption);

    cmdParser.addPositionalArgument("database", "Path to a .minidb database file (optional).");
    cmdParser.process(app);

    const QStringList args = cmdParser.positionalArguments();
    QString dbPath = args.isEmpty() ? QString() : args.first();

    // ── CLI mode ──────────────────────────────────────────────────────
    if (cmdParser.isSet(cliOption)) {
        return runCli(dbPath);
    }
    // ── GUI mode ──────────────────────────────────────────────────────
    app.setStyleSheet(minidb::theme::darkStyleSheet());

    // Set default font
    QFont uiFont(minidb::theme::FONT_UI, minidb::theme::FONT_SIZE_UI);
    app.setFont(uiFont);

    minidb::MainWindow mainWindow;

    // If a database path was given on the command line, open it
    if (!dbPath.isEmpty()) {
        mainWindow.openDatabaseFile(dbPath);
    }

    mainWindow.show();
    return app.exec();
}
