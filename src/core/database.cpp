#include "database.h"
#include "core/errors.h"
#include "sql/lexer.h"
#include "sql/parser.h"
#include "sql/result.h"
#include "storage/pager.h"
#include "storage/catalog.h"
#include "engine/executor.h"

#include <QElapsedTimer>
#include <QFileInfo>

namespace minidb {

// ── Private implementation ────────────────────────────────────────────

struct Database::Impl {
    QString             filePath;
    std::unique_ptr<Pager>    pager;
    std::unique_ptr<Catalog>  catalog;
    std::unique_ptr<Executor> executor;
    bool                      opened = false;

    void init() {
        catalog = std::make_unique<Catalog>(*pager);
        catalog->load();
        executor = std::make_unique<Executor>(*catalog, *pager);
    }
};

// ── Database ──────────────────────────────────────────────────────────

Database::Database() : m_impl(std::make_unique<Impl>()) {}

Database::~Database() {
    close();
}

bool Database::create(const QString& filePath) {
    close();

    try {
        m_impl->pager = std::make_unique<Pager>(filePath, true /* create */);
        m_impl->filePath = filePath;
        m_impl->init();
        m_impl->catalog->save();
        m_impl->pager->flushAll();
        m_impl->opened = true;
        return true;
    } catch (const DbError&) {
        m_impl->pager.reset();
        return false;
    }
}

bool Database::open(const QString& filePath) {
    close();

    try {
        m_impl->pager = std::make_unique<Pager>(filePath, false /* open existing */);
        m_impl->filePath = filePath;
        m_impl->init();
        m_impl->opened = true;
        return true;
    } catch (const DbError&) {
        m_impl->pager.reset();
        return false;
    }
}

void Database::close() {
    if (m_impl->opened) {
        if (m_impl->catalog)
            m_impl->catalog->save();
        if (m_impl->pager) {
            m_impl->pager->flushAll();
            m_impl->pager->close();
        }
        m_impl->executor.reset();
        m_impl->catalog.reset();
        m_impl->pager.reset();
        m_impl->opened = false;
    }
}

bool Database::isOpen() const {
    return m_impl->opened;
}

QueryResult Database::execute(const QString& sql) {
    if (!m_impl->opened) {
        return QueryResult::error("No database is open.");
    }

    try {
        // Phase 1: Lex
        Lexer lexer(sql);
        QVector<Token> tokens = lexer.tokenize();

        // Phase 2: Parse
        Parser parser(tokens);
        auto stmt = parser.parse();

        if (!stmt) {
            return QueryResult::error("Failed to parse SQL statement.");
        }

        // Phase 3: Execute
        QueryResult result = m_impl->executor->execute(*stmt);

        // Persist changes after DML/DDL
        m_impl->catalog->save();
        m_impl->pager->flushAll();

        return result;

    } catch (const ParseError& e) {
        return QueryResult::error(e.message());
    } catch (const ExecutionError& e) {
        return QueryResult::error(e.message());
    } catch (const StorageError& e) {
        return QueryResult::error(e.message());
    } catch (const CatalogError& e) {
        return QueryResult::error(e.message());
    } catch (const ConstraintError& e) {
        return QueryResult::error(e.message());
    } catch (const DbError& e) {
        return QueryResult::error(e.message());
    } catch (const std::exception& e) {
        return QueryResult::error(QString("Internal error: %1").arg(e.what()));
    }
}

QString Database::filePath() const {
    return m_impl->filePath;
}

Catalog* Database::catalog() const {
    return m_impl->catalog.get();
}

} // namespace minidb
