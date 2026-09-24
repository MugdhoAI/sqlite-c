#ifndef SQLITE_H
#define SQLITE_H

#include <stddef.h>
#include <stdint.h>

#include "pager.h"

#define SQLITE_USERNAME_MAX 32
#define SQLITE_EMAIL_MAX 255
#define SQLITE_ROW_SIZE (sizeof(uint32_t) + (SQLITE_USERNAME_MAX + 1U) + (SQLITE_EMAIL_MAX + 1U))

typedef enum {
    SQLITE_OK = 0,
    SQLITE_TABLE_FULL,
    SQLITE_INVALID_ID,
    SQLITE_INVALID_USERNAME,
    SQLITE_INVALID_EMAIL,
    SQLITE_DUPLICATE_ID,
    SQLITE_IO_ERROR
} SqliteResult;

typedef struct {
    int id;
    char username[SQLITE_USERNAME_MAX + 1];
    char email[SQLITE_EMAIL_MAX + 1];
} Row;

typedef struct Table Table;

Table *table_create(size_t capacity);
Table *table_open(const char *filename, SqliteResult *result);
void table_destroy(Table *table);
SqliteResult table_flush(Table *table);
void table_print_tree(const Table *table);

SqliteResult table_insert(Table *table, const Row *row);
size_t table_size(const Table *table);
const Row *table_row_at(const Table *table, size_t index);
SqliteResult table_find(const Table *table, int id, Row *row);

const char *sqlite_result_string(SqliteResult result);

#endif
