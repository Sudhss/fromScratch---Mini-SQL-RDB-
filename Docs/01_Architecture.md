# Mini SQL RDB: Comprehensive Architectural Documentation

This document serves as the exhaustive architectural guide and design rationale behind the Mini SQL Relational Database (MiniSQL RDB) built from scratch in C++ and Qt6. Every subsystem, design choice, and algorithmic implementation is discussed in deep detail to answer not just *what* was built, but *how* and *why* it was built that way.

---

## 1. System Overview

The MiniSQL RDB is a standalone, embedded relational database management system. It does not rely on any third-party database libraries (like SQLite) or parser generators (like Flex/Bison). The entire stack—from parsing text to flushing bytes to disk—is custom-built. 

### 1.1 Core Components

1. **Storage Engine**: Manages disk I/O, page allocation, buffer pooling, and binary layout of records.
2. **SQL Frontend**: A hand-written Lexer and Recursive Descent Parser that transforms raw SQL strings into an Abstract Syntax Tree (AST).
3. **Query Engine**: A Volcano-style iterator model executor that plans queries and evaluates expressions.
4. **GUI / UI Layer**: A Qt6-based Integrated Development Environment (IDE) featuring a syntax-highlighted editor, schema explorer, and results grid, alongside a CLI mode.

---

## 2. Storage Engine Architecture

The Storage Engine is the lowest layer of the database. Its primary responsibility is durability and efficient retrieval of data from the underlying `.minidb` file. Real databases do not write arbitrary variable-length strings sequentially to disk; they manage data in fixed-size chunks called **Pages**.

### 2.1 The Page Layout (Slotted Pages)

**Why fixed-size pages?** 
Operating systems and storage devices read and write data in blocks (typically 4KB or 8KB). By aligning our database pages to the OS page size (4096 bytes), we minimize I/O overhead and prevent fragmentation.

**How is a page structured?**
We use the **Slotted Page Architecture**. This is the industry standard (used by PostgreSQL, SQLite, and InnoDB) for storing variable-length records (like `VARCHAR`).

```cpp
constexpr uint32_t PAGE_SIZE = 4096;
constexpr uint32_t PAGE_HEADER_SIZE = 16;
constexpr uint32_t SLOT_SIZE = 4; // 2 bytes offset, 2 bytes length
```

A slotted page is divided into three sections:
1. **Header (16 bytes)**: Stores the `pageType`, `recordCount`, pointers to the start and end of the free space, and a `nextPageId` for linking pages together.
2. **Slot Array**: An array of `[offset, length]` pairs that grows *downwards* from the end of the header. Each slot points to a specific record in the page.
3. **Record Data**: The actual binary rows, which grow *upwards* from the very end of the page (byte 4095).

**Why this layout?**
If records were simply appended back-to-back, deleting a 50-byte record in the middle of a page would leave a 50-byte "hole". Shifting all subsequent records to fill the hole would change their physical offsets, invalidating any index pointers to those records. 
With slotted pages, an index only stores the `(PageID, SlotIndex)`. If a record is shifted during defragmentation, only the 2-byte offset in the slot array needs to be updated. External index pointers remain valid.

### 2.2 The Buffer Pool (Pager)

**Why not read from disk on every query?**
Disk I/O is orders of magnitude slower than memory access. The database must cache recently used pages in memory.

**How does it work?**
The `Pager` class acts as a Buffer Pool Manager. It maintains a cache of `Page` objects. When the executor requests `readPage(5)`, the Pager:
1. Checks the in-memory cache (a `QHash<uint32_t, CacheEntry>`).
2. If found (Cache Hit), returns the page immediately.
3. If not found (Cache Miss), allocates a buffer, reads exactly 4096 bytes from `QFile` at offset `5 * 4096`, and inserts it into the cache.

**Eviction Policy (LRU):**
The cache has a maximum capacity (e.g., 256 pages = 1MB). When full, the Pager uses a Least Recently Used (LRU) policy via a `QLinkedList` to evict the oldest, unaccessed page. If the evicted page is marked "dirty" (modified), it is synchronously flushed to disk before being removed from memory.

### 2.3 Row Serialization and Record Format

**Why a custom binary format?**
Text formats (like CSV or JSON) require expensive parsing and converting string representations to integers on every read. Binary formats are compact and can be mapped directly to CPU registers.

**How are rows serialized?**
The `RecordSerializer` takes a `Row` (a vector of `Value` variants) and a `TableSchema` and produces a `QByteArray`.
1. **Null Bitmap**: The first bytes of a record are a bitmap (1 bit per column). If a bit is set, the column is `NULL`, and takes up 0 bytes in the data section.
2. **Data Section**: 
   - `INT` and `DATE`: 4 bytes, Big-Endian.
   - `FLOAT`: 8 bytes, IEEE 754 standard.
   - `BOOL`: 1 byte (0x00 or 0x01).
   - `VARCHAR`: A 2-byte length prefix followed by UTF-8 bytes. (Null-terminators are not used, saving 1 byte per string and allowing fast length lookups).

### 2.4 Heap Files (Tables)

Tables are organized as **Heap Files**—a collection of unordered data pages. Because pages are fixed at 4KB, a single table will span multiple pages. The pages are connected via a singly-linked list using the `nextPageId` in the page header.

When `INSERT INTO` is called:
1. The `Table` class requests the first page from the `Pager`.
2. It attempts to insert the serialized record.
3. If the page returns `-1` (no space), the Table traverses the `nextPageId` link.
4. If it reaches the end of the list, it asks the Pager to `allocatePage()`, links it, and inserts the record there.

### 2.5 Indexing: B+Tree Implementation

**Why B+Tree instead of a Hash Index or Binary Search Tree?**
- BSTs can become unbalanced (O(N) search) and their node pointers cause severe disk thrashing (random I/O).
- Hash Indexes only support equality lookups (`WHERE id = 5`), not range scans (`WHERE id BETWEEN 5 AND 10`).
- A B+Tree is shallow (usually 3-4 levels deep even for billions of rows), perfectly aligned to 4KB pages, and supports both point lookups and range scans.

**How is the B+Tree structured?**
- **Internal Nodes**: Store only routing keys and pointers (Page IDs) to child pages. Because they don't store row data, a 4KB page can hold hundreds of routing keys, leading to massive fan-out.
- **Leaf Nodes**: Store the actual keys and the `RowId` (PageID + SlotIndex). Crucially, leaf nodes have `next_leaf_page` pointers, creating a doubly-linked list at the bottom. 
- **Range Scans**: To execute `SELECT * WHERE id > 100`, the database searches the tree to find `100`, then simply traverses the linked list of leaf pages rightward, without ever traversing back up the tree.

---

## 3. SQL Frontend Architecture

The SQL Frontend is responsible for understanding human-readable SQL and converting it into a machine-executable Abstract Syntax Tree (AST).

### 3.1 Lexer (Tokenizer)

The Lexer scans the raw SQL string character-by-character. It groups characters into logical `Token` units.

**Design Choices:**
- **Hand-written State Machine**: Instead of using Regex (which is slow and hard to debug for complex parsing), we use a switch-case state machine reading a `QString`.
- **Lookahead**: When the lexer sees `<`, it peeks at the next character. If it's `=`, it emits `LESS_THAN_EQUALS`; if `>` it emits `NOT_EQUALS`; otherwise it emits `LESS_THAN`.
- **String Literals**: Handled cleanly by scanning until the matching closing quote, escaping embedded quotes.

### 3.2 Recursive Descent Parser

**Why Recursive Descent?**
Parser generators (Yacc/Bison) generate unreadable C code and provide notoriously bad error messages ("Syntax error near token"). A hand-written Recursive Descent parser allows us to throw precise exceptions: "Parse error at line 2 col 15: Expected 'INTO' after 'INSERT'".

**How it works:**
The parser consists of mutually recursive functions for every non-terminal in the SQL grammar: `parseStatement()`, `parseSelect()`, `parseWhereClause()`, etc.

```cpp
std::unique_ptr<Statement> Parser::parseSelect() {
    expect(TokenType::SELECT);
    auto columns = parseSelectColumnList();
    expect(TokenType::FROM);
    auto table = parseTableRef();
    // ...
}
```

### 3.3 Pratt Parsing for Expressions (Precedence Climbing)

Parsing expressions like `A + B * C = D OR E IS NULL` is extremely complex due to operator precedence. Recursive descent struggles here without creating deeply nested, redundant functions.

**How Pratt Parsing Solves This:**
We associate a "binding power" (precedence level) with every operator. 
- `*` has power 60
- `+` has power 50
- `=` has power 40
- `AND` has power 30
- `OR` has power 20

The `parseExpression(precedence)` function loops, consuming tokens as long as their binding power is strictly greater than the current context. This cleanly builds a correct AST where `*` binds tighter than `+`.

---

## 4. Query Engine Architecture

Once the AST is built, it must be executed. MiniSQL RDB uses the **Volcano Iterator Model**.

### 4.1 The Planner

The Planner acts as the bridge between the AST and the Executor. Currently, it is a **rule-based optimizer**.
- **Index Lookups**: If the AST contains a `WhereClause` with a `BinaryExpr` utilizing the `EQUALS` operator against a `PRIMARY KEY` column, the Planner outputs a `QueryPlan` setting `scanType = INDEX_LOOKUP`.
- **Full Table Scans**: If the condition does not hit an index, it defaults to `FULL_SCAN`.

### 4.2 The Evaluator

The `Evaluator` is a stateless engine that evaluates an `Expression` AST node against a specific `Row`.

**NULL Propagation Logic:**
Database logic uses Three-Valued Logic (True, False, Unknown/Null). The Evaluator carefully enforces this:
- `5 + NULL = NULL`
- `TRUE AND NULL = NULL`
- `FALSE AND NULL = FALSE` (Short-circuiting)
- `NULL = NULL` is `NULL`, not `TRUE`. (This is why `IS NULL` exists).

**LIKE Pattern Matching:**
Handled via a recursive backtracking algorithm. `%` branches into two recursive calls: one consuming the wildcard, and one consuming the character but keeping the wildcard.

### 4.3 The Executor

The `Executor` orchestrates the pipeline. For a `SELECT` statement, it follows this strict logical sequence:
1. **FROM & JOIN**: Resolves the base table. Iterates through the table via `TableIterator`. For `JOIN`s, it executes a Nested Loop Join (for every row in Table A, loop through Table B).
2. **WHERE**: Passes the combined row to the `Evaluator`. If `evaluateCondition` returns false, the row is discarded.
3. **GROUP BY & Aggregates**: Sorts or hashes the rows into groups. For each group, it evaluates aggregate functions (`COUNT`, `SUM`).
4. **HAVING**: Filters the aggregated groups.
5. **ORDER BY**: Uses `std::sort` with a custom lambda that evaluates the `ORDER BY` expression for two rows and compares the resulting `Value` objects.
6. **SELECT (Projection)**: Evaluates the specific columns requested in the output.
7. **LIMIT**: Truncates the final result set.

The output is packed into a `QueryResult` object, which is totally detached from the storage engine, allowing the GUI to render it safely.

---

## 5. Qt Integration & GUI Architecture

The user interfaces with the database either via a command-line interface or a rich Qt6 IDE.

### 5.1 Clean Separation of Concerns

The GUI layer (`src/ui/`) is entirely decoupled from the database internals. The only connection point is the `Database::execute(QString sql)` method. The UI sends a string, and receives a `QueryResult`. The UI knows nothing about slotted pages or AST nodes.

### 5.2 Model-View Architecture

The `ResultsTable` widget leverages Qt's Model/View architecture. 
Instead of creating thousands of `QTableWidgetItem` objects (which consumes massive memory and CPU), we implemented a custom `QAbstractTableModel` (`ResultsTableModel`). 
This model holds a reference to the `QueryResult` and overrides `data(index, role)`. Qt only asks the model for data for the cells currently visible on screen. This allows the database to instantly display a result set of 100,000 rows without any UI freezing.

### 5.3 Syntax Highlighting

The `SqlHighlighter` inherits from `QSyntaxHighlighter`. It attaches to the `QTextDocument` of the `QueryEditor`. As the user types, Qt triggers a re-highlight of the changed block. We use `QRegularExpression` matching to rapidly apply Catppuccin color formats to SQL keywords, strings, and comments, providing an instant visual feedback loop.

---

## 6. Design Constraints and Future Work

As a "Mini" RDB, certain compromises were made for educational clarity and scope constraint:

1. **Transactions (ACID)**: Version 1 implements durability via `flushAll()`, but does not implement a Write-Ahead Log (WAL) or undo/redo logs. Therefore, it lacks Atomicity and Isolation guarantees during concurrent access or sudden power failure. 
2. **Concurrency**: The database is single-threaded. Multiple queries execute serially. A future version would introduce `QReadWriteLock` on the Pager to allow concurrent reads and exclusive writes.
3. **Cost-Based Optimization**: The query planner is rule-based. It does not gather table statistics (cardinality, histograms) to make dynamic join-order decisions.

## Summary

The MiniSQL RDB demonstrates a massive vertical slice of computer science disciplines:
- **Systems Programming**: Managing disk I/O, raw binary buffers, and memory caches.
- **Compiler Theory**: Lexical analysis, context-free grammars, and abstract syntax trees.
- **Algorithms**: B+Tree traversal, Nested Loop Joins, and recursive pattern matching.
- **UI/UX**: Asynchronous data models, syntax highlighting, and responsive desktop application design.
