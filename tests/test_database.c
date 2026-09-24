#include "sqlite.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static Row make_row(int id)
{
    Row row = {0};
    row.id = id;
    snprintf(row.username, sizeof(row.username), "user%d", id);
    snprintf(row.email, sizeof(row.email), "user%d@example.com", id);
    return row;
}

static void test_insert_and_read(void)
{
    const char *filename = "build/test_database.db";
    remove(filename);

    SqliteResult result;
    Table *table = table_open(filename, &result);
    assert(table != NULL);
    assert(result == SQLITE_OK);

    Row first = make_row(1);
    Row second = make_row(2);

    assert(table_insert(table, &first) == SQLITE_OK);
    assert(table_insert(table, &second) == SQLITE_OK);
    assert(table_size(table) == 2);

    table_destroy(table);

    table = table_open(filename, &result);
    assert(table != NULL);
    assert(table_size(table) == 2);

    const Row *row = table_row_at(table, 1);
    assert(row != NULL);
    assert(row->id == 2);
    assert(strcmp(row->username, "user2") == 0);
    assert(strcmp(row->email, "user2@example.com") == 0);

    table_destroy(table);
    remove(filename);
}

static void test_btree_split_and_order(void)
{
    const char *filename = "build/test_btree.db";
    remove(filename);

    SqliteResult result;
    Table *table = table_open(filename, &result);
    assert(table != NULL);

    for (int id = 40; id >= 1; --id) {
        Row row = make_row(id);
        assert(table_insert(table, &row) == SQLITE_OK);
    }

    assert(table_size(table) == 40);

    Row duplicate = make_row(13);
    assert(table_insert(table, &duplicate) == SQLITE_DUPLICATE_ID);
    assert(table_size(table) == 40);

    table_destroy(table);

    table = table_open(filename, &result);
    assert(table != NULL);
    assert(table_size(table) == 40);

    for (size_t i = 0; i < table_size(table); ++i) {
        const Row *row = table_row_at(table, i);
        assert(row != NULL);
        assert(row->id == (int)i + 1);
    }

    table_destroy(table);
    remove(filename);
}

static void test_lookup(void)
{
    const char *filename = "build/test_lookup.db";
    remove(filename);

    SqliteResult result;
    Table *table = table_open(filename, &result);
    assert(table != NULL);

    Row row = make_row(23);
    assert(table_insert(table, &row) == SQLITE_OK);

    Row found = {0};
    assert(table_find(table, 23, &found) == SQLITE_OK);
    assert(found.id == 23);
    assert(strcmp(found.username, "user23") == 0);

    assert(table_find(table, 99, &found) != SQLITE_OK);

    table_destroy(table);
    remove(filename);
}

static void test_validation(void)
{
    const char *filename = "build/test_validation.db";
    remove(filename);

    SqliteResult result;
    Table *table = table_open(filename, &result);
    assert(table != NULL);

    Row invalid_id = make_row(0);
    assert(table_insert(table, &invalid_id) == SQLITE_INVALID_ID);

    Row empty_username = make_row(1);
    empty_username.username[0] = '\0';
    assert(table_insert(table, &empty_username) == SQLITE_INVALID_USERNAME);

    Row duplicate = make_row(2);
    assert(table_insert(table, &duplicate) == SQLITE_OK);
    assert(table_insert(table, &duplicate) == SQLITE_DUPLICATE_ID);

    table_destroy(table);
    remove(filename);
}

int main(void)
{
    test_insert_and_read();
    test_btree_split_and_order();
    test_lookup();
    test_validation();
    return 0;
}
