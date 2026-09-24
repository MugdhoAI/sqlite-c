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

static void test_select(void)
{
    Statement statement;
    assert(parse_statement("SELECT * FROM users;", &statement) == PARSER_OK);
    assert(statement.type == STATEMENT_SELECT);
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
    test_select();
    test_invalid_input();
    return 0;
}
