# sqlite-c

A SQLite inspired relational database engine built from scratch in C.

This project is a learning focused implementation of the core ideas behind a relational database. It starts with a small interactive shell and will grow into a persistent database engine with page based storage, B tree indexing, a SQL parser, query execution, and automated tests.

The goal is not to reproduce the full SQLite feature set. The goal is to understand and implement the important mechanisms that make a database engine work.

## Current status

Phase 1 and the first storage milestone are complete. The repository has a portable C build, a small database API, an interactive shell, automated tests, and a page based persistence layer.

The current engine stores a fixed number of rows in a database file through a 4096 byte page. The next storage milestone is the B tree layer, which will remove the current single page limitation.

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
    insert <id> <username> <email>
    select

Example:

    db > insert 1 alice alice@example.com
    db > insert 2 bob bob@example.com
    db > select
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
- [ ] B tree table storage
- [ ] SQL tokenizer and parser
- [ ] Query execution
- [ ] Stronger error handling
- [ ] Integration tests
- [ ] Benchmarks and profiling
- [ ] Documentation of storage internals

## License

MIT
