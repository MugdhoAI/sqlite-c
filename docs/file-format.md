# sqlite-c File Format Specification

**Status:** Describes the current implementation on the audited main branch. This is not a compatibility promise. The format is private to sqlite-c and is not SQLite compatible.

## General properties

- Page size: 4096 bytes.
- Page numbers: zero based.
- Page zero: metadata.
- Pages one and above: B tree nodes.
- Maximum page count: 100, including page zero.
- Integer encoding: unsigned 32 bit little endian for persisted IDs, pointers, counts, and metadata values.
- Allocation: append only. There is no free page list or page reuse.
- Format version: 1.

The pager loads and flushes complete pages. It does not provide checksums, page level versioning, journaling, or crash recovery.

## Metadata page (page 0)

| Offset | Size | Field | Meaning |
| --- | ---: | --- | --- |
| 0 | 4 | Magic | 0x53514c43 |
| 4 | 4 | Format version | 1 |
| 8 | 4 | Root page | B tree root page number |
| 12 | 4 | Row count | Number of stored rows |

The remaining bytes are currently unused and initialized to zero for a new database.

A new database starts with root page 1, containing an empty leaf.

## Common B tree node header

| Offset | Size | Field |
| --- | ---: | --- |
| 0 | 1 | Node type: 1 leaf, 2 internal |
| 1 | 3 | Unused padding |
| 4 | 4 | Parent page number |
| 8 | 4 | Cell count |

The parent page number is zero for the root. Page number zero is reserved for metadata and is also used as the null page pointer.

## Leaf node layout

Leaf nodes use a 16 byte header. Offset 12 stores the next leaf page number. Zero means there is no next leaf.

Cells begin at offset 16. Each cell is one fixed size row:

| Cell offset | Size | Field |
| --- | ---: | --- |
| 0 | 4 | ID, unsigned 32 bit little endian |
| 4 | 33 | Username bytes, including terminator |
| 37 | 256 | Email bytes, including terminator |

Each row cell occupies 293 bytes. With a 16 byte header, a 4096 byte page holds at most 13 rows.

IDs are accepted by the table API as positive C `int` values. The B tree stores their 32 bit representation. Persisted rows are deserialized into the fixed width username and email buffers, with the final byte of each buffer forced to a null terminator.

## Internal node layout

Internal nodes also use a 16 byte header. Offset 12 stores the rightmost child page number.

Cells begin at offset 16. Each cell is 8 bytes:

| Cell offset | Size | Field |
| --- | ---: | --- |
| 0 | 4 | Child page number |
| 4 | 4 | Separator key |

A separator key is the maximum key stored in the child referenced by that cell. The rightmost child has no separator cell of its own.

The page layout can physically hold up to 510 cells. The current implementation does not split internal nodes, so practical tree growth is limited by the root's ability to accept additional leaf children and by the 100 page database limit.

## Tree invariants checked on open

The current validator checks, among other things:

- The root points to an existing non metadata page.
- Every referenced node has the expected parent page.
- Node types and cell counts are within supported bounds.
- Leaf keys are positive, within the supported signed integer range, and strictly increasing.
- Internal child key ranges are ordered.
- Stored separators equal the maximum key in the child to their left.
- Leaf next pointers match the in order traversal of the tree.
- Every allocated non metadata page is reachable from the root.
- The number of rows found equals the metadata row count.

These checks do not make the format self repairing and do not guarantee detection of every possible corruption pattern.

## Durability limitations

Pages are written with standard C file I/O and `fflush`. There is no transaction journal, WAL, checksum, atomic commit protocol, or explicit stable storage synchronization. The format therefore makes no crash durability guarantee.
