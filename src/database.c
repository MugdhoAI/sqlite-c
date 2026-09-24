#include "sqlite.h"

#include <stdlib.h>
#include <string.h>

struct Table {
    Row *rows;
    size_t size;
    size_t capacity;
};

static int valid_text(const char *value, size_t max_length)
{
    if (value == NULL || value[0] == '\0') {
        return 0;
    }

    return strlen(value) <= max_length;
}

static int has_id(const Table *table, int id)
{
    for (size_t i = 0; i < table->size; ++i) {
        if (table->rows[i].id == id) {
            return 1;
        }
    }

    return 0;
}

Table *table_create(size_t capacity)
{
    if (capacity == 0) {
        return NULL;
    }

    Table *table = malloc(sizeof(*table));
    if (table == NULL) {
        return NULL;
    }

    table->rows = calloc(capacity, sizeof(*table->rows));
    if (table->rows == NULL) {
        free(table);
        return NULL;
    }

    table->size = 0;
    table->capacity = capacity;

    return table;
}

void table_destroy(Table *table)
{
    if (table == NULL) {
        return;
    }

    free(table->rows);
    free(table);
}

SqliteResult table_insert(Table *table, const Row *row)
{
    if (table == NULL || row == NULL) {
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

    if (has_id(table, row->id)) {
        return SQLITE_DUPLICATE_ID;
    }

    if (table->size >= table->capacity) {
        return SQLITE_TABLE_FULL;
    }

    table->rows[table->size] = *row;
    ++table->size;

    return SQLITE_OK;
}

size_t table_size(const Table *table)
{
    return table == NULL ? 0 : table->size;
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
        return "table full";
    case SQLITE_INVALID_ID:
        return "id must be a positive integer";
    case SQLITE_INVALID_USERNAME:
        return "username must be non empty and at most 32 characters";
    case SQLITE_INVALID_EMAIL:
        return "email must be non empty and at most 255 characters";
    case SQLITE_DUPLICATE_ID:
        return "id already exists";
    default:
        return "unknown error";
    }
}
