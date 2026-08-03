#pragma once

#include "core/types.h"
#include "sql/result.h"
#include <QString>
#include <memory>

namespace minidb {

// Forward declarations
class Pager;
class Catalog;
class Executor;

// ── Database ──────────────────────────────────────────────────────────
// Top-level façade — the single entry point for the GUI and CLI.
// Owns the storage engine (Pager + Catalog) and the query executor.
//
// Usage:
//   Database db;
//   db.create("my_database.minidb");
//   auto result = db.execute("CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(50));");

class Database {
public:
    Database();
    ~Database();

    // Lifecycle
    bool create(const QString& filePath);   // Create a new .minidb file
    bool open(const QString& filePath);     // Open an existing .minidb file
    void close();                           // Flush and close
    bool isOpen() const;

    // Query execution — the main API
    QueryResult execute(const QString& sql);

    // Accessors
    QString filePath() const;
    Catalog* catalog() const;               // For schema explorer

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace minidb
