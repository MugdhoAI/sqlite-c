# sqlite-c

A SQLite inspired relational database engine built from scratch in C.

This project is a learning focused implementation of the core ideas behind a relational database. It starts with a small interactive shell and will grow into a persistent database engine with page based storage, B tree indexing, a SQL parser, query execution, and automated tests.

The goal is not to reproduce the full SQLite feature set. The goal is to understand and implement the important mechanisms that make a database engine work.

## Current status

Phase 1 is complete: the repository has a portable C build, a small database API, an interactive shell, and automated tests.

The first working version stores rows in memory. Persistent pages and the B tree storage layer will be introduced in the next phase.

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
- [ ] Page based storage
- [ ] Persistent database files
- [ ] B tree table storage
- [ ] SQL tokenizer and parser
- [ ] Query execution
- [ ] Stronger error handling
- [ ] Integration tests
- [ ] Benchmarks and profiling
- [ ] Documentation of storage internals

## License

MIT
