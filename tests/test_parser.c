#include "parser.h"

#include <assert.h>
#include <stdio.h>
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

static void test_lowercase_insert_and_select(void)
{
    Statement statement;
    assert(parse_statement(
               "insert into users values (8, 'carol', 'carol@example.com');",
               &statement) == PARSER_OK);
    assert(statement.type == STATEMENT_INSERT);
    assert(statement.row.id == 8);
    assert(strcmp(statement.row.username, "carol") == 0);
    assert(strcmp(statement.row.email, "carol@example.com") == 0);

    assert(parse_statement("select * from users;", &statement) == PARSER_OK);
    assert(statement.type == STATEMENT_SELECT);
    assert(parse_statement("select * from users where id = 9;", &statement) ==
           PARSER_OK);
    assert(statement.where_id == 9);
}

static void test_legacy_insert(void)
{
    Statement statement;
    assert(parse_statement("insert 5 bob bob@example.com", &statement) ==
           PARSER_OK);
    assert(statement.type == STATEMENT_INSERT);
    assert(statement.row.id == 5);
    assert(strcmp(statement.row.username, "bob") == 0);
    assert(strcmp(statement.row.email, "bob@example.com") == 0);
}

static void test_legacy_insert_semicolon_is_not_part_of_email(void)
{
    Statement statement;
    assert(parse_statement("insert 5 bob bob@example.com;", &statement) ==
           PARSER_OK);
    assert(statement.type == STATEMENT_INSERT);
    assert(strcmp(statement.row.email, "bob@example.com") == 0);

    assert(parse_statement("insert 5 bob bob@example.com; garbage",
                           &statement) == PARSER_SYNTAX_ERROR);
    assert(parse_statement("insert 5 bob bob@example.com garbage",
                           &statement) == PARSER_SYNTAX_ERROR);
}

static void test_meta_help_and_exit(void)
{
    Statement statement;
    assert(parse_statement(".help", &statement) == PARSER_OK);
    assert(statement.type == META_HELP);
    assert(parse_statement(".exit", &statement) == PARSER_OK);
    assert(statement.type == META_EXIT);
}

static void test_parse_rejects_null_and_empty(void)
{
    Statement statement;
    assert(parse_statement(NULL, &statement) == PARSER_SYNTAX_ERROR);
    assert(parse_statement("SELECT * FROM users;", NULL) ==
           PARSER_SYNTAX_ERROR);
    assert(parse_statement("", &statement) == PARSER_SYNTAX_ERROR);
}

static void test_parse_rejects_zero_id(void)
{
    Statement statement;
    assert(parse_statement(
               "INSERT INTO users VALUES (0, 'alice', 'alice@example.com');",
               &statement) == PARSER_SYNTAX_ERROR);
    assert(parse_statement("SELECT * FROM users WHERE id = 0;", &statement) ==
           PARSER_SYNTAX_ERROR);
    assert(parse_statement("SELECT * FROM users WHERE id = -1;", &statement) ==
           PARSER_SYNTAX_ERROR);
}

static void test_insert_rejects_empty_fields(void)
{
    Statement statement;
    assert(parse_statement("INSERT INTO users VALUES (1, '', 'a@b.com');",
                           &statement) == PARSER_SYNTAX_ERROR);
    assert(parse_statement("INSERT INTO users VALUES (1, 'alice', '');",
                           &statement) == PARSER_SYNTAX_ERROR);
    assert(parse_statement("INSERT INTO users VALUES (1, alice, 'a@b.com');",
                           &statement) == PARSER_SYNTAX_ERROR);
}

static void test_insert_field_length_limits(void)
{
    Statement statement;
    char input[600];
    char field[257];

    memset(field, 'a', 32);
    field[32] = '\0';
    snprintf(input, sizeof(input),
             "INSERT INTO users VALUES (1, '%s', 'a@b.com');", field);
    assert(parse_statement(input, &statement) == PARSER_OK);
    assert(strlen(statement.row.username) == 32U);

    memset(field, 'a', 33);
    field[33] = '\0';
    snprintf(input, sizeof(input),
             "INSERT INTO users VALUES (1, '%s', 'a@b.com');", field);
    assert(parse_statement(input, &statement) == PARSER_SYNTAX_ERROR);

    memset(field, 'e', 255);
    field[255] = '\0';
    snprintf(input, sizeof(input),
             "INSERT INTO users VALUES (1, 'alice', '%s');", field);
    assert(parse_statement(input, &statement) == PARSER_OK);
    assert(strlen(statement.row.email) == 255U);

    memset(field, 'e', 256);
    field[256] = '\0';
    snprintf(input, sizeof(input),
             "INSERT INTO users VALUES (1, 'alice', '%s');", field);
    assert(parse_statement(input, &statement) == PARSER_SYNTAX_ERROR);
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
    test_lowercase_insert_and_select();
    test_legacy_insert();
    test_legacy_insert_semicolon_is_not_part_of_email();
    test_meta_help_and_exit();
    test_parse_rejects_null_and_empty();
    test_parse_rejects_zero_id();
    test_insert_rejects_empty_fields();
    test_insert_field_length_limits();
    return 0;
}
