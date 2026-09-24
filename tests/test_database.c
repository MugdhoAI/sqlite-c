#include "sqlite.h"

#include <assert.h>
#include <string.h>

static Row make_row(int id, const char *username, const char *email)
{
    Row row = {0};
    row.id = id;
    strncpy(row.username, username, SQLITE_USERNAME_MAX);
    strncpy(row.email, email, SQLITE_EMAIL_MAX);
    row.username[SQLITE_USERNAME_MAX] = '\0';
    row.email[SQLITE_EMAIL_MAX] = '\0';
    return row;
}

static void test_insert_and_read(void)
{
    Table *table = table_create(2);
    assert(table != NULL);

    Row first = make_row(1, "alice", "alice@example.com");
    Row second = make_row(2, "bob", "bob@example.com");

    assert(table_insert(table, &first) == SQLITE_OK);
    assert(table_insert(table, &second) == SQLITE_OK);
    assert(table_size(table) == 2);

    const Row *row = table_row_at(table, 1);
    assert(row != NULL);
    assert(row->id == 2);
    assert(strcmp(row->username, "bob") == 0);
    assert(strcmp(row->email, "bob@example.com") == 0);

    table_destroy(table);
}

static void test_validation(void)
{
    Table *table = table_create(4);
    assert(table != NULL);

    Row invalid_id = make_row(0, "alice", "alice@example.com");
    assert(table_insert(table, &invalid_id) == SQLITE_INVALID_ID);

    Row empty_username = make_row(1, "", "alice@example.com");
    assert(table_insert(table, &empty_username) == SQLITE_INVALID_USERNAME);

    Row duplicate = make_row(2, "alice", "alice@example.com");
    assert(table_insert(table, &duplicate) == SQLITE_OK);
    assert(table_insert(table, &duplicate) == SQLITE_DUPLICATE_ID);

    table_destroy(table);
}

static void test_capacity(void)
{
    Table *table = table_create(1);
    assert(table != NULL);

    Row first = make_row(1, "alice", "alice@example.com");
    Row second = make_row(2, "bob", "bob@example.com");

    assert(table_insert(table, &first) == SQLITE_OK);
    assert(table_insert(table, &second) == SQLITE_TABLE_FULL);

    table_destroy(table);
}

int main(void)
{
    test_insert_and_read();
    test_validation();
    test_capacity();
    return 0;
}
