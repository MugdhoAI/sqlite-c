#ifndef BTREE_H
#define BTREE_H

#include <stddef.h>

#include "pager.h"
#include "sqlite.h"

typedef enum {
    BTREE_OK = 0,
    BTREE_INVALID_ARGUMENT,
    BTREE_IO_ERROR,
    BTREE_DUPLICATE_KEY,
    BTREE_TABLE_FULL,
    BTREE_CORRUPT
} BTreeResult;

typedef struct BTree BTree;

BTree *btree_open(Pager *pager, BTreeResult *result);
void btree_close(BTree *tree);
BTreeResult btree_flush(BTree *tree);

BTreeResult btree_insert(BTree *tree, const Row *row);
BTreeResult btree_find(const BTree *tree, int id, Row *row);
BTreeResult btree_read_all(const BTree *tree, Row *rows, size_t capacity, size_t *count);
void btree_print(const BTree *tree);
size_t btree_size(const BTree *tree);

const char *btree_result_string(BTreeResult result);

#endif
