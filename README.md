# sqlite-c

A SQLite inspired relational database engine built from scratch in C.

This project is a learning focused implementation of the core ideas behind a relational database. It starts with a small interactive shell and will grow into a persistent database engine with page based storage, B tree indexing, a SQL parser, query execution, and automated tests.

The goal is not to reproduce the full SQLite feature set. The goal is to understand and implement the important mechanisms that make a database engine work.

## Current status

Phase 1 and the first storage milestone are complete. The repository has a portable C build, a small database API, an interactive shell, automated tests, and a page based persistence layer.

The current engine stores rows in a persistent page based B tree. The shell accepts a small SQL subset and scans the linked leaf pages in key order. The implementation deliberately keeps the supported SQL surface small so the storage engine remains the focus.

## Build

Requirements:

- C11 compiler
- make

Build the project:

    make

Run the shell:

    ./build/sqlite-c

Run tests:

    make test

## Shell

The current shell supports:

    .help
    .exit
    INSERT INTO users VALUES (id, 'username', 'email');
    SELECT * FROM users;

The original short insert form is still accepted for quick experiments.

Example:

    db > INSERT INTO users VALUES (1, 'alice', 'alice@example.com');
    db > INSERT INTO users VALUES (2, 'bob', 'bob@example.com');
    db > SELECT * FROM users;
    1 | alice | alice@example.com
    2 | bob | bob@example.com

## Architecture

The implementation is intentionally divided into small layers:

    shell
      |
      v
    database API
      |
      v
    table and row storage
      |
      v
    persistent storage
      |
      v
    B tree

The architecture will evolve as each storage layer is implemented.

## Roadmap

- [x] Project foundation
- [x] Interactive shell
- [x] In memory rows and table
- [x] Basic tests
- [x] Page based storage
- [x] Persistent database files
- [x] B tree table storage
- [x] SQL statement parser
- [x] Query execution
- [ ] Stronger error handling
- [ ] Integration tests
- [ ] Benchmarks and profiling
- [ ] Documentation of storage internals

## License

MIT
