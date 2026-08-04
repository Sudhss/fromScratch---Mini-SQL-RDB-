# Mini SQL RDB

A fully custom mini relational database management system built from scratch in C++ and Qt6. It features a complete SQL frontend, a robust storage engine, a query executor, and a polished dark-themed GUI (along with a CLI mode).

Zero external database dependencies. No `flex`/`bison`. No SQLite under the hood.

## Features

- **Storage Engine**: 4KB slotted pages, LRU buffer pool manager, heap file organization, and B+Tree indexing for primary keys.
- **SQL Parser**: Hand-written lexer and recursive descent parser supporting a practical subset of SQL (DDL and DML).
- **Query Engine**: Volcano-style query execution pipeline with expression evaluation, query planning (index vs. full scan), joins, and aggregates.
- **Dual Interface**:
  - **IDE-like GUI**: Features a SQL query editor with syntax highlighting, a schema explorer tree, and a data results table view.
  - **Interactive CLI**: A command-line REPL for executing queries and managing the database via meta-commands.
- **Data Types**: `INT`, `FLOAT`, `VARCHAR`, `BOOL`, `DATE`.
- **Aesthetics**: Modern Catppuccin Mocha-inspired dark theme for the Qt UI.

## Project Structure

The project follows a clean architectural layout:

```
src/
├── core/       # Foundational types (DataType, Value, Row), error hierarchy, and the Database façade
├── storage/    # Disk I/O (Pages, Pager, Records, Tables, Catalog, BTree)
├── sql/        # SQL Tokenizer, Lexer, AST nodes, Parser, and QueryResult
├── engine/     # Expression Evaluator, Planner, and Executor
├── ui/         # Main Window, Query Editor, Results Table, Schema Explorer, Status Bar
└── theme/      # Dark theme definitions and SQL Syntax Highlighter
```

## Build Instructions

This project uses CMake and requires Qt 6.

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Usage

### GUI Mode
Simply run the executable without arguments to launch the IDE-like interface:
```bash
./MiniSQLRDB
```
Or provide a database file path to open it automatically:
```bash
./MiniSQLRDB my_database.minidb
```

### CLI Mode
Pass the `--cli` flag to start the interactive command-line interface:
```bash
./MiniSQLRDB --cli my_database.minidb
```

Inside the CLI, you can use `.help` to see available meta-commands (like `.tables`, `.open`, `.create`) or type raw SQL terminated by a semicolon.

## Supported SQL

- **DDL**: `CREATE TABLE`, `DROP TABLE`, `ALTER TABLE ADD COLUMN`
- **DML**: `INSERT INTO`, `SELECT`, `UPDATE`, `DELETE`
- **Clauses**: `WHERE`, `ORDER BY`, `LIMIT`, `GROUP BY`, `HAVING`
- **Joins**: `INNER JOIN`, `LEFT JOIN`
- **Aggregates**: `COUNT()`, `SUM()`, `AVG()`, `MIN()`, `MAX()`
- **Operators**: `=`, `!=`, `<`, `>`, `<=`, `>=`, `AND`, `OR`, `NOT`, `LIKE`, `IS NULL`, `IS NOT NULL`, `BETWEEN`, `IN`
- **Utility**: `SHOW TABLES`, `DESCRIBE <table>` 

## License
No license it is open source
