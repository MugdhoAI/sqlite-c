# Architecture

The database is built as a small set of layers with explicit responsibilities.

## Shell

`src/main.c` owns the interactive loop. It reads one command, sends it to the parser, executes the resulting statement, and prints the result.

## Parser

`src/parser.c` converts the supported SQL statements into typed statements.

The current SQL surface is intentionally small:

    INSERT INTO users VALUES (id, 'username', 'email');
    SELECT * FROM users;

The parser is kept separate from storage so the SQL layer can grow without changing the page format.

## Table API

`src/database.c` provides the public table operations used by the shell and tests. It validates rows and maintains an ordered in memory view for result printing.

The persistent source of truth is the B tree.

## B tree

`src/btree.c` stores rows in ordered leaf pages.

Each leaf contains fixed size row cells and a link to the next leaf. Internal pages contain child page numbers and separator keys. The root page remains stable while leaf splits create new child pages.

The current implementation supports a root internal node and multiple leaf pages. The internal root has enough fanout for the current 100 page database limit, so internal node splitting is intentionally deferred until the storage limit is expanded.

## Pager

`src/pager.c` maps the database file into fixed 4096 byte pages and caches loaded pages in memory. Modified pages are written back when the table is flushed or closed.

Page zero stores database metadata. The remaining pages contain B tree nodes.

## File format

The database file is private to this project and is not intended to be compatible with SQLite files.

The first page stores:

    magic
    format version
    root page number
    row count

B tree pages use a compact fixed layout so the implementation can focus on storage mechanics without introducing a general record format too early.

## Data flow

    command
       |
       v
    parser
       |
       v
    table API
       |
       v
    B tree
       |
       v
    pager
       |
       v
    database file

This separation is the foundation for later additions such as WHERE clauses, deletion, transactions, indexes, and a more complete SQL grammar.
