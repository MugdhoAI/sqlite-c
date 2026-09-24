#include "sqlite.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_ROW_CAPACITY ((PAGE_SIZE - sizeof(uint32_t)) / SQLITE_ROW_SIZE)

struct Table {
    Row *rows;
    size_t size;
    size_t capacity;
    Pager *pager;
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

static void write_u32(unsigned char *destination, uint32_t value)
{
    destination[0] = (unsigned char)(value & 0xffU);
    destination[1] = (unsigned char)((value >> 8U) & 0xffU);
    destination[2] = (unsigned char)((value >> 16U) & 0xffU);
    destination[3] = (unsigned char)((value >> 24U) & 0xffU);
}

static uint32_t read_u32(const unsigned char *source)
{
    return ((uint32_t)source[0]) |
           ((uint32_t)source[1] << 8U) |
           ((uint32_t)source[2] << 16U) |
           ((uint32_t)source[3] << 24U);
}

static void serialize_row(unsigned char *destination, const Row *row)
{
    write_u32(destination, (uint32_t)row->id);
    memcpy(destination + sizeof(uint32_t), row->username, SQLITE_USERNAME_MAX + 1U);
    memcpy(destination + sizeof(uint32_t) + SQLITE_USERNAME_MAX + 1U,
           row->email, SQLITE_EMAIL_MAX + 1U);
}

static void deserialize_row(Row *row, const unsigned char *source)
{
    row->id = (int)read_u32(source);
    memcpy(row->username, source + sizeof(uint32_t), SQLITE_USERNAME_MAX + 1U);
    memcpy(row->email,
           source + sizeof(uint32_t) + SQLITE_USERNAME_MAX + 1U,
           SQLITE_EMAIL_MAX + 1U);
    row->username[SQLITE_USERNAME_MAX] = '\0';
    row->email[SQLITE_EMAIL_MAX] = '\0';
}

static Table *allocate_table(size_t capacity)
{
    if (capacity == 0) {
        return NULL;
    }

    Table *table = calloc(1, sizeof(*table));
    if (table == NULL) {
        return NULL;
    }

    table->rows = calloc(capacity, sizeof(*table->rows));
    if (table->rows == NULL) {
        free(table);
        return NULL;
    }

    table->capacity = capacity;
    return table;
}

Table *table_create(size_t capacity)
{
    return allocate_table(capacity);
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

    size_t capacity = PAGE_ROW_CAPACITY;
    if (capacity == 0) {
        pager_close(pager);
        *result = SQLITE_IO_ERROR;
        return NULL;
    }

    Table *table = allocate_table(capacity);
    if (table == NULL) {
        pager_close(pager);
        *result = SQLITE_IO_ERROR;
        return NULL;
    }

    table->pager = pager;

    if (pager->num_pages == 0) {
        return table;
    }

    unsigned char *page = pager_get_page(pager, 0, &pager_result);
    if (page == NULL) {
        table_destroy(table);
        *result = SQLITE_IO_ERROR;
        return NULL;
    }

    uint32_t stored_size = read_u32(page);
    if (stored_size > capacity) {
        table_destroy(table);
        *result = SQLITE_IO_ERROR;
        return NULL;
    }

    table->size = stored_size;
    for (size_t i = 0; i < table->size; ++i) {
        deserialize_row(&table->rows[i],
                       page + sizeof(uint32_t) + (i * SQLITE_ROW_SIZE));
    }

    return table;
}

SqliteResult table_flush(Table *table)
{
    if (table == NULL || table->pager == NULL) {
        return SQLITE_IO_ERROR;
    }

    PagerResult pager_result;
    unsigned char *page = pager_get_page(table->pager, 0, &pager_result);
    if (page == NULL) {
        return SQLITE_IO_ERROR;
    }

    memset(page, 0, PAGE_SIZE);
    write_u32(page, (uint32_t)table->size);

    for (size_t i = 0; i < table->size; ++i) {
        serialize_row(page + sizeof(uint32_t) + (i * SQLITE_ROW_SIZE), &table->rows[i]);
    }

    return pager_flush(table->pager, 0) == PAGER_OK ? SQLITE_OK : SQLITE_IO_ERROR;
}

void table_destroy(Table *table)
{
    if (table == NULL) {
        return;
    }

    if (table->pager != NULL) {
        (void)table_flush(table);
        pager_close(table->pager);
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
    case SQLITE_IO_ERROR:
        return "database I/O error";
    default:
        return "unknown error";
    }
}
