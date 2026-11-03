# Architecture

## Scope

sqlite-c is a small educational relational database engine written in C11. It uses a project specific file format and is not compatible with SQLite database files.

The implementation separates command parsing, the table API, B tree storage, and page based file access.

## Data flow

```text
interactive shell (src/main.c)
        |
        v
SQL subset parser (src/parser.c)
        |
        v
table API (src/database.c)
        |
        v
B tree (src/btree.c)
        |
        v
pager (src/pager.c)
        |
        v
database file
```

## Shell

`src/main.c` reads one command at a time into a 512 byte buffer, rejects commands that exceed the buffer, parses the command, and dispatches the resulting statement. The shell supports `.help`, `.btree`, and `.exit`.

The SQL surface is limited to INSERT, SELECT all rows, and SELECT by ID. The parser also retains a legacy short INSERT form.

## Parser

`src/parser.c` recognizes a small set of command patterns using `sscanf`, string comparisons, and numeric conversion with `strtol`. It checks that supported commands have no unexpected trailing input. It is not a general SQL grammar or tokenizer.

The parser returns a typed `Statement`. It does not access storage.

## Table API

`src/database.c` exposes the public `Table` interface declared in `include/sqlite.h`.

Responsibilities include validating row fields, opening and closing the pager and B tree, translating storage errors, maintaining an ordered in memory row array, and exposing insert, lookup, iteration, flush, and close operations.

The B tree is the persistent source of truth. The in memory row array is a second representation used by the table iteration API. It is allocated with a fixed capacity of `TABLE_MAX_PAGES * 13` rows and rebuilt from the B tree on open.

## B tree

`src/btree.c` stores rows in sorted leaf pages. Internal pages store child page numbers and separator keys. A separator is the maximum key in the child to its left.

The current tree has at most two levels: one internal root and its leaf children. When the initial root leaf splits, the original root page is converted into an internal root and the old leaf contents are copied to a newly allocated child page. Later leaf splits add another child to that root.

Internal node splitting and deeper levels are not implemented. A full internal root causes insertion to return a capacity error.

Leaf pages are linked in key order. Full table reads start at the leftmost leaf and follow this linked list. Point lookup descends from the root using separator keys.

On open, validation checks node types and counts, page references, parent pointers, key ordering, separator values, leaf chain order, reachability of allocated pages, and the metadata row count. This is meaningful structural validation, not a guarantee that every possible malformed byte sequence or corrupt row field will be detected.

## Pager

`src/pager.c` owns the database `FILE *`, tracks file length and page count, and keeps loaded pages in a fixed array of 100 page pointers. Pages are allocated and loaded on demand. A flush writes an entire 4096 byte page and calls `fflush`.

The pager does not implement a replacement policy, dirty page tracking, transactions, write ahead logging, or an operating system durability barrier such as `fsync`.

## Page allocation and capacity

Page zero is reserved for metadata. B tree pages are allocated by taking the current pager page count and appending a page. There is no free list or page reuse. The total database limit is 100 pages, including metadata.

## Persistence and error boundaries

The B tree writes metadata and flushes loaded pages during explicit flush and close. The pager also attempts to flush loaded pages during close. A successful `fflush` is not a guarantee that data has reached stable storage after power loss or an operating system crash.

The engine has no transaction boundary or rollback mechanism. A failed operation after mutating cached pages may leave the in memory state unsuitable for continued use. Failure atomicity must be designed and tested before stronger reliability claims are made.

## Tests and build

The Makefile builds the shell, database tests, pager tests, parser tests, and a benchmark. The database tests cover insertion, persistence, ordering, leaf splits, duplicate keys, lookup, row validation, and selected malformed database structures. Parser tests cover supported commands, trailing input, and numeric overflow. Pager tests cover a basic page write and read round trip. A shell integration test checks oversized command handling.

GitHub Actions runs the test suite, a strict warning build, a benchmark smoke test, and a test build with AddressSanitizer and UndefinedBehaviorSanitizer.

## Current boundaries

The implementation does not provide multi level B tree growth, page reuse, variable length records, general SQL, UPDATE, DELETE, transactions, rollback, WAL, crash recovery, concurrent access, or SQLite file compatibility.
