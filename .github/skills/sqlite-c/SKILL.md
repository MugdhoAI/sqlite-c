# sqlite-c Project Skill

## Purpose

This skill defines how to understand, maintain, extend, test, and review the sqlite-c repository. It is specific to this project. It must not be reused as generic database guidance without checking the target implementation.

sqlite-c is an educational relational database engine written in C11. It uses its own file format and is not compatible with SQLite database files or intended as a replacement for SQLite.

## Source of truth

Use this priority order when resolving project facts:

1. Current source code on the target branch.
2. Tests that exercise the behavior.
3. Build configuration and CI workflows.
4. Current architecture and file format documents.
5. README and release notes.
6. Older plans, discussions, or model generated summaries.

If documentation conflicts with code, report the conflict and update documentation only after confirming the intended behavior. Never turn a planned feature into a claim about the current implementation.

## Current verified architecture

The command path is:

```text
src/main.c -> src/parser.c -> src/database.c -> src/btree.c -> src/pager.c -> database file
```

- `main.c`: 512 byte interactive input buffer, shell command dispatch, output.
- `parser.c`: recognizes the supported INSERT and SELECT forms plus shell meta commands. Uses string matching and `sscanf`; it is not a general SQL parser.
- `database.c`: public Table API, row validation, error translation, sorted in memory row view, storage lifecycle.
- `btree.c`: fixed width row serialization, leaf and internal node operations, point lookup, ordered leaf traversal, structural validation, metadata.
- `pager.c`: fixed size page cache backed by standard C file I/O.

See `docs/architecture.md` for the full verified architecture and `docs/file-format.md` for the persisted byte layout.

## Current implementation facts

- C11.
- 4096 byte pages.
- 100 pages maximum, including metadata page zero.
- Page zero stores magic, format version, root page number, and row count.
- Rows have a positive integer ID, username buffer of 33 bytes, and email buffer of 256 bytes.
- Persisted row cells occupy 293 bytes.
- Leaf pages have a 16 byte header and hold at most 13 rows.
- Internal pages have a 16 byte header and 8 byte child/separator cells.
- The B tree supports a root and leaf children only. Internal node splitting is not implemented.
- Page allocation appends using the pager page count. There is no page reuse.
- Supported SQL is INSERT, SELECT all, and SELECT by ID. A legacy short INSERT form remains.
- The engine has no transactions, rollback, WAL, crash recovery, concurrency, or SQLite file compatibility.
- The table API keeps an in memory ordered row array in addition to the persistent B tree.

## Engineering rules

1. Preserve C11 compatibility and the strict compiler warning policy.
2. Keep changes focused. Avoid unrelated refactoring in correctness fixes.
3. Make ownership and lifetime of allocated memory explicit.
4. Check allocation, file positioning, reads, writes, flushes, and close results.
5. Treat persisted bytes as untrusted input. Validate lengths, counts, pointers, types, and ranges before indexing or deserializing.
6. Do not assume a failed storage operation leaves cached pages unchanged.
7. Preserve B tree ordering, parent pointers, separator semantics, leaf chain order, metadata row count, and page reachability.
8. Any file format change must update the format specification and include compatibility or versioning decisions.
9. Do not claim transactions, ACID properties, crash durability, concurrency safety, SQLite compatibility, or production readiness without implementation and tests that establish those guarantees.
10. Do not modify the default branch, publish a release, merge a pull request, or create public issues without explicit user approval.

## Required workflow for every change

1. State the concrete problem and reproduce it.
2. Trace the affected call path and relevant invariants.
3. Define expected behavior and failure behavior.
4. Write a regression test that fails before the fix where practical.
5. Implement the smallest coherent change.
6. Run the focused test, then `make clean && make test`.
7. Run sanitizer checks using the repository CI flags.
8. Run the strict build and benchmark smoke test when relevant.
9. Inspect the diff for scope, ownership, error handling, and documentation.
10. Report exactly what passed, what failed, and what was not run.

Never claim a test passed unless its output was observed.

## Current baseline commands

```sh
make clean
make
make test
make benchmark
```

Sanitizer command used by CI:

```sh
make clean && make test CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer"
```

## Known audit priorities

These are risks and limitations identified during the initial source audit. They are not all confirmed defects:

1. Define and test behavior for files whose length is not a multiple of 4096 bytes.
2. Make insert failure behavior safe when a leaf split succeeds locally but parent insertion fails.
3. Redesign page allocation before adding deeper B tree levels.
4. Test the maximum page and root capacity boundaries.
5. Add I/O failure testing for seek, read, write, flush, and close paths.
6. Expand malformed file tests beyond the currently covered metadata, node count, and child reference cases.
7. Revisit error mapping so corruption, missing rows, invalid arguments, and I/O failures remain distinguishable.
8. Define the supported row encoding and validate persisted text fields consistently.

## Phase boundaries

### Phase 1: Understanding and audit
Complete architecture tracing, verify claims against source and tests, document the file format, and record limitations and risks. Do not implement features during this phase.

### Phase 2: Correctness and reliability
Address confirmed correctness defects, failure handling, input validation, and regression coverage. Do not add new SQL features as a substitute for storage correctness.

### Phase 3: Multi level B tree and allocation
Design page allocation and internal node splitting together. Specify invariants first, then implement and test recursive splits, root growth, traversal, and validation.

Later phases may cover SQL expansion, transactions, benchmarking, contributor experience, and community growth. Do not detail or implement them until earlier phases are complete.

## Review checklist

- Does the change preserve the documented file format or explicitly version it?
- Are all page numbers and cell counts bounds checked before access?
- Are parent pointers, separators, and leaf links correct after every split?
- Can an error leave the in memory tree inconsistent?
- Does reopening the database reproduce the expected rows?
- Are tests checking both success and failure behavior?
- Are sanitizer and strict warning results observed?
- Are documentation claims limited to verified behavior?

## Documentation discipline

Update architecture and file format documentation in the same change as relevant implementation changes. Explain complex algorithms while implementing them, especially leaf splitting, internal splitting, and page allocation. Keep limitations visible. Use reproducible examples and measured benchmark results rather than unsupported performance claims.
