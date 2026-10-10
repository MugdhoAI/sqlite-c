# sqlite-c

A small SQLite inspired relational database engine built from scratch in C11.

The project focuses on the storage mechanisms behind a database rather than reproducing SQLite's full feature set. It includes persistent storage built around fixed size pages, B tree table storage, a small SQL parser, an interactive shell, automated tests, a benchmark, and sanitizer coverage.

The database file format is intentionally project specific and is not compatible with SQLite database files.

## Why this project exists

Database engines hide several systems problems behind a simple API:

How rows are represented on disk
How fixed size pages are loaded and flushed
How records remain ordered as the tree grows
How a B tree routes lookups and handles leaf splits
How persistence survives closing and reopening the database
How a SQL command becomes an operation on stored data
How low level C code is tested for correctness and memory errors

sqlite-c makes those mechanisms explicit in a small codebase that can be built, tested, and inspected end to end.

## Demo

Build and launch the interactive shell:

```bash
make
./build/sqlite-c
```

Example session:

```text
db > INSERT INTO users VALUES (1, 'alice', 'alice@example.com');
db > INSERT INTO users VALUES (2, 'bob', 'bob@example.com');
db > SELECT * FROM users;
1 | alice | alice@example.com
2 | bob | bob@example.com
db > SELECT * FROM users WHERE id = 2;
2 | bob | bob@example.com
db > .btree
...
db > .exit
```

![sqlite-c shell demo](docs/images/demo.png)

The database is persisted to `build/sqlite.db`, so data can be written, the process closed, and the database reopened later.

![sqlite-c persistence demonstration](docs/images/persistence.png)

## What is implemented

| Component | Implementation |
| --- | --- |
| Language | C11 |
| SQL surface | INSERT and SELECT, including `WHERE id = ...` |
| Storage | Persistent fixed size pages |
| Page size | 4096 bytes |
| Index structure | B tree with leaf pages and an internal root |
| Persistence | Database metadata and B tree pages flushed to disk |
| Shell | Interactive command line interface |
| Validation | Row ID, username, email, duplicate key checks |
| Testing | Database, B tree, pager, and parser tests |
| Benchmarking | 1,000 row insertion benchmark |
| CI | Strict compiler warnings, benchmark smoke test, ASan and UBSan |

The current storage design supports up to 100 pages. Internal node splitting beyond the current root is intentionally deferred until the page capacity model is expanded.

## Architecture

```text
                 Interactive shell
                       │
                       ▼
                    Parser
                       │
                       ▼
                  Table API
                       │
                       ▼
                     B tree
                  ┌────┴────┐
                  │         │
             Leaf pages  Root index
                  │
                  ▼
                 Pager
                  │
                  ▼
             Database file
```

![sqlite-c architecture](docs/images/architecture.svg)

### Shell

`src/main.c` owns the interactive loop. It reads commands, invokes the parser, executes the resulting statement, and prints results.

### Parser

`src/parser.c` converts the supported SQL commands into typed statements. Keeping parsing separate from storage means the SQL surface can grow without changing the page format.

### Table API

`src/database.c` validates rows and exposes the public table operations used by the shell and tests. It also maintains the ordered in memory result view used by the shell.

The persistent source of truth is the B tree.

### B tree

`src/btree.c` stores rows in ordered leaf pages.

Leaf pages contain fixed size row cells and a pointer to the next leaf. The current internal root stores child page numbers and separator keys. When a leaf becomes full, it is split and the new sibling is inserted into its parent.

The root remains stable when the first leaf split occurs by converting the original root page into an internal node and moving the previous leaf contents into a new child page.

![B tree split from the current implementation](docs/images/btree-split.png)

### Pager

`src/pager.c` manages fixed size 4096 byte pages backed by the database file. Pages are loaded into memory on demand and flushed back to disk when required.

Page zero stores database metadata. The remaining pages contain B tree nodes.

### File format

The database format is private to sqlite-c.

The metadata page currently stores:

```text
magic
format version
root page number
row count
```

B tree pages use a compact fixed layout designed to keep the storage implementation understandable.

## Performance

The benchmark inserts 1,000 rows through the public table API and measures the insertion phase. It is intended for repeatable local comparisons rather than as a claim of production database performance.

The current write path keeps modified pages in the pager cache during inserts and persists them during an explicit flush. This avoids performing a file write and fflush for every inserted row.

Run it with:

```bash
make benchmark
```

![sqlite-c benchmark](docs/images/benchmark.png)

Record benchmark results only from the current build and environment. Do not compare numbers across machines without noting the compiler, operating system, and build flags.

## Testing and verification

The test suite covers:

Insert and read behavior
Persistence across close and reopen
B tree leaf splitting
Ordered traversal
Duplicate key rejection
Point lookup
Input validation
Pager behavior
SQL parser behavior

Run the full test suite:

```bash
make test
```

Run the benchmark:

```bash
make benchmark
```

CI also builds with strict warnings and runs the tests with:

AddressSanitizer
UndefinedBehaviorSanitizer
`-Wall`
`-Wextra`
`-Wpedantic`
`-Wconversion`
`-Wshadow`
`-Werror`

## Installation

Requirements:

C11 compatible compiler
GNU Make

Build:

```bash
make
```

Run:

```bash
./build/sqlite-c
```

Clean generated files:

```bash
make clean
```

## Supported commands

Meta commands:

```text
.help
.btree
.exit
```

SQL subset:

```sql
INSERT INTO users VALUES (1, 'alice', 'alice@example.com');

SELECT * FROM users;

SELECT * FROM users WHERE id = 1;
```

A legacy short insert form is also retained for quick experiments.

## Engineering trade offs

### Fixed size pages

Using 4096 byte pages keeps the pager and B tree layout predictable. The trade off is that the current implementation has a deliberately small capacity and does not attempt to model SQLite's full page management system.

### Fixed size rows

Rows use fixed size username and email fields. This makes serialization straightforward and keeps B tree cells simple, at the cost of unused space for short values.

### Project specific file format

The on disk format is intentionally simple instead of matching SQLite's format. This keeps the implementation focused on database engineering concepts rather than compatibility work.

### Root only internal routing

The current implementation supports an internal root with multiple leaf children. Splitting deeper internal levels is deferred. This is a conscious scope boundary rather than an attempt to claim a production ready B tree implementation.

### Small SQL grammar

The parser handles only the commands needed by the current storage engine. A larger SQL grammar would add complexity without improving the project's core storage objective.

## Repository layout

```text
sqlite-c/
├── include/       public interfaces
├── src/           database, B tree, pager, parser, shell
├── tests/         database, pager, and parser tests
├── bench/         insertion benchmark
├── docs/          architecture notes and visual documentation
├── .github/       CI workflow
├── Makefile       build, test, and benchmark targets
└── README.md
```

## Future work

The next meaningful extensions are:

Deeper B tree internal node splitting
More complete SQL parsing
DELETE and UPDATE
Transactions and stronger durability guarantees
More comprehensive integration tests
Profiling and repeatable benchmark reporting
More detailed documentation of page and node layouts
Additional indexes and query execution paths

## License

MIT
