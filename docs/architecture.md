# Architecture

sqlite-c is organized as a small set of layers with explicit responsibilities. The design keeps SQL handling separate from storage so the database format can evolve independently from the command surface.

## Data flow

```text
interactive command
        |
        v
      parser
        |
        v
    table API
        |
        v
      B-tree
     /      \
leaf pages  internal root
     |
     v
    pager
     |
     v
database file
```

## Shell

`src/main.c` owns the interactive loop. It reads one command, sends it to the parser, executes the resulting statement, and prints the result.

Supported SQL currently includes:

```sql
INSERT INTO users VALUES (id, 'username', 'email');
SELECT * FROM users;
SELECT * FROM users WHERE id = 1;
```

Meta commands include `.help`, `.btree`, and `.exit`.

## Parser

`src/parser.c` converts supported commands into typed statements.

The parser is deliberately small. It is responsible for recognizing the supported command forms; validation and storage errors are handled by the lower layers.

Keeping parsing separate from storage means new SQL operations can be added without changing the page format.

## Table API

`src/database.c` provides the public table operations used by the shell and tests.

Responsibilities include:

- validating row fields
- rejecting invalid and duplicate IDs
- inserting rows into the B-tree
- maintaining the ordered in-memory result view
- exposing point lookup and row iteration
- translating lower-level errors into public results

The persistent source of truth is the B-tree.

## B-tree

`src/btree.c` stores rows in ordered leaf pages.

Leaf pages contain:

- a node type
- a parent page number
- a row count
- a next-leaf pointer
- fixed-size row cells

The internal root contains child page numbers and separator keys.

When a leaf becomes full, sqlite-c:

1. allocates a sibling page
2. combines the existing rows with the new row
3. splits the rows into ordered left and right leaves
4. links the new leaf into the leaf chain
5. inserts the new sibling into the parent
6. updates separators and flushes the affected pages

When the root leaf splits for the first time, the original root page is converted into an internal node and its previous contents are moved into a child page. This keeps the root page number stable.

The current implementation intentionally stops at a root internal node. Deeper internal-node splitting is deferred until the storage capacity model is expanded.

## Pager

`src/pager.c` manages the database file as fixed-size 4096-byte pages.

The pager:

- opens or creates the database file
- loads pages on demand
- keeps loaded pages in memory
- flushes modified pages to disk
- enforces the current 100-page limit

Page zero stores database metadata. B-tree nodes occupy the remaining pages.

## File format

The database format is project-specific and is not intended to be compatible with SQLite.

The metadata page stores:

```text
magic
format version
root page number
row count
```

B-tree pages use a compact fixed layout. Rows are serialized directly into fixed-size cells using the project-defined `Row` structure.

## Testing

The storage tests exercise persistence, ordering, B-tree leaf splitting, duplicate detection, point lookup, and validation.

The parser and pager have separate test binaries. CI additionally builds with strict warnings and runs the suite under AddressSanitizer and UndefinedBehaviorSanitizer.

## Design boundary

The project intentionally prioritizes understandable storage mechanics over SQL completeness. Features such as transactions, deeper B-tree levels, variable-length records, and a broader query planner are left for future iterations.
