#include "parser.h"

#include <assert.h>
#include <string.h>

static void test_insert(void)
{
    Statement statement;
    assert(parse_statement(
        "INSERT INTO users VALUES (7, 'alice', 'alice@example.com');",
        &statement) == PARSER_OK);
    assert(statement.type == STATEMENT_INSERT);
    assert(statement.row.id == 7);
    assert(strcmp(statement.row.username, "alice") == 0);
    assert(strcmp(statement.row.email, "alice@example.com") == 0);
}

static void test_insert_rejects_trailing_input(void)
{
    Statement statement;
    assert(parse_statement(
        "INSERT INTO users VALUES (7, 'alice', 'alice@example.com'); garbage",
        &statement) == PARSER_SYNTAX_ERROR);
}

static void test_select(void)
{
    Statement statement;
    assert(parse_statement("SELECT * FROM users;", &statement) == PARSER_OK);
    assert(statement.type == STATEMENT_SELECT);
}

static void test_select_by_id(void)
{
    Statement statement;
    assert(parse_statement("SELECT * FROM users WHERE id = 42;", &statement) == PARSER_OK);
    assert(statement.type == STATEMENT_SELECT);
    assert(statement.where_id == 42);
}

static void test_select_rejects_trailing_input(void)
{
    Statement statement;
    assert(parse_statement("SELECT * FROM users WHERE id = 42 garbage", &statement) ==
           PARSER_SYNTAX_ERROR);
    assert(parse_statement("SELECT * FROM users WHERE id = 42abc", &statement) ==
           PARSER_SYNTAX_ERROR);
}

static void test_select_allows_trailing_whitespace(void)
{
    Statement statement;
    assert(parse_statement("SELECT * FROM users;   ", &statement) == PARSER_OK);
}

static void test_meta_command(void)
{
    Statement statement;
    assert(parse_statement(".btree", &statement) == PARSER_OK);
    assert(statement.type == META_BTREE);
}

static void test_numeric_overflow_is_rejected(void)
{
    Statement statement;
    assert(parse_statement(
        "INSERT INTO users VALUES (2147483648, 'alice', 'alice@example.com');",
        &statement) == PARSER_SYNTAX_ERROR);
    assert(parse_statement("SELECT * FROM users WHERE id = 999999999999999999999;",
                           &statement) == PARSER_SYNTAX_ERROR);
}

static void test_invalid_input(void)
{
    Statement statement;
    assert(parse_statement("SELECT username FROM users;", &statement) ==
           PARSER_SYNTAX_ERROR);
}

int main(void)
{
    test_insert();
    test_insert_rejects_trailing_input();
    test_select();
    test_select_by_id();
    test_select_rejects_trailing_input();
    test_select_allows_trailing_whitespace();
    test_meta_command();
    test_invalid_input();
    test_numeric_overflow_is_rejected();
    return 0;
}
