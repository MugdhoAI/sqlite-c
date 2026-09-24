#include "sqlite.h"

#include "btree.h"

#include <stdlib.h>
#include <string.h>

#define TABLE_ROW_CAPACITY (TABLE_MAX_PAGES * 13U)

struct Table {
    Pager *pager;
    BTree *tree;
    Row *rows;
    size_t size;
};

static int valid_text(const char *value, size_t max_length)
{
    if (value == NULL || value[0] == '\0') {
        return 0;
    }

    return strlen(value) <= max_length;
}

static Table *allocate_table(void)
{
    Table *table = calloc(1, sizeof(*table));
    if (table == NULL) {
        return NULL;
    }

    table->rows = calloc(TABLE_ROW_CAPACITY, sizeof(*table->rows));
    if (table->rows == NULL) {
        free(table);
        return NULL;
    }

    return table;
}

Table *table_create(size_t capacity)
{
    if (capacity == 0 || capacity > TABLE_ROW_CAPACITY) {
        return NULL;
    }

    return allocate_table();
}

Table *table_open(const char *filename, SqliteResult *result)
{
    if (filename == NULL || result == NULL) {
        return NULL;
    }

    *result = SQLITE_OK;

    PagerResult pager_result;
    Pager *pager = pager_open(filename, &pager_result);
    if (pager == NULL) {
        *result = SQLITE_IO_ERROR;
        return NULL;
    }

    Table *table = allocate_table();
    if (table == NULL) {
        pager_close(pager);
        *result = SQLITE_IO_ERROR;
        return NULL;
    }

    BTreeResult tree_result;
    BTree *tree = btree_open(pager, &tree_result);
    if (tree == NULL) {
        pager_close(pager);
        free(table->rows);
        free(table);
        *result = SQLITE_IO_ERROR;
        return NULL;
    }

    table->pager = pager;
    table->tree = tree;

    BTreeResult read_result =
        btree_read_all(tree, table->rows, TABLE_ROW_CAPACITY, &table->size);
    if (read_result != BTREE_OK) {
        table_destroy(table);
        *result = SQLITE_IO_ERROR;
        return NULL;
    }

    return table;
}

SqliteResult table_flush(Table *table)
{
    if (table == NULL || table->tree == NULL) {
        return SQLITE_IO_ERROR;
    }

    return btree_flush(table->tree) == BTREE_OK ? SQLITE_OK : SQLITE_IO_ERROR;
}

void table_print_tree(const Table *table)
{
    if (table != NULL) {
        btree_print(table->tree);
    }
}

void table_destroy(Table *table)
{
    if (table == NULL) {
        return;
    }

    if (table->tree != NULL) {
        btree_close(table->tree);
    }

    if (table->pager != NULL) {
        pager_close(table->pager);
    }

    free(table->rows);
    free(table);
}

SqliteResult table_insert(Table *table, const Row *row)
{
    if (table == NULL || table->tree == NULL || row == NULL) {
        return SQLITE_INVALID_ID;
    }

    if (row->id <= 0) {
        return SQLITE_INVALID_ID;
    }

    if (!valid_text(row->username, SQLITE_USERNAME_MAX)) {
        return SQLITE_INVALID_USERNAME;
    }

    if (!valid_text(row->email, SQLITE_EMAIL_MAX)) {
        return SQLITE_INVALID_EMAIL;
    }

    if (table->size >= TABLE_ROW_CAPACITY) {
        return SQLITE_TABLE_FULL;
    }

    BTreeResult result = btree_insert(table->tree, row);
    if (result != BTREE_OK) {
        switch (result) {
        case BTREE_DUPLICATE_KEY:
            return SQLITE_DUPLICATE_ID;
        case BTREE_TABLE_FULL:
            return SQLITE_TABLE_FULL;
        case BTREE_IO_ERROR:
        case BTREE_CORRUPT:
            return SQLITE_IO_ERROR;
        default:
            return SQLITE_INVALID_ID;
        }
    }

    size_t insert_at = 0;
    while (insert_at < table->size && table->rows[insert_at].id < row->id) {
        ++insert_at;
    }

    for (size_t i = table->size; i > insert_at; --i) {
        table->rows[i] = table->rows[i - 1U];
    }

    table->rows[insert_at] = *row;
    ++table->size;
    return SQLITE_OK;
}

size_t table_size(const Table *table)
{
    return table == NULL ? 0U : table->size;
}

const Row *table_row_at(const Table *table, size_t index)
{
    if (table == NULL || index >= table->size) {
        return NULL;
    }

    return &table->rows[index];
}

const char *sqlite_result_string(SqliteResult result)
{
    switch (result) {
    case SQLITE_OK:
        return "ok";
    case SQLITE_TABLE_FULL:
        return "database capacity reached";
    case SQLITE_INVALID_ID:
        return "id must be a positive integer";
    case SQLITE_INVALID_USERNAME:
        return "username must be non empty and at most 32 characters";
    case SQLITE_INVALID_EMAIL:
        return "email must be non empty and at most 255 characters";
    case SQLITE_DUPLICATE_ID:
        return "id already exists";
    case SQLITE_IO_ERROR:
        return "database I/O error";
    default:
        return "unknown error";
    }
}
