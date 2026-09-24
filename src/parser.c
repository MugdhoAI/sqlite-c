#include "parser.h"

#include <stdio.h>
#include <string.h>

static int parse_insert(const char *input, Statement *statement)
{
    int matched = sscanf(
        input,
        "INSERT INTO users VALUES (%d, '%32[^']', '%255[^']')",
        &statement->row.id,
        statement->row.username,
        statement->row.email
    );

    if (matched != 3) {
        matched = sscanf(
            input,
            "insert into users values (%d, '%32[^']', '%255[^']')",
            &statement->row.id,
            statement->row.username,
            statement->row.email
        );
    }

    if (matched == 3 && statement->row.id > 0) {
        statement->type = STATEMENT_INSERT;
        return 1;
    }

    return 0;
}

static int parse_legacy_insert(const char *input, Statement *statement)
{
    int matched = sscanf(
        input,
        "insert %d %32s %255s",
        &statement->row.id,
        statement->row.username,
        statement->row.email
    );

    if (matched == 3 && statement->row.id > 0) {
        statement->type = STATEMENT_INSERT;
        return 1;
    }

    return 0;
}

ParserResult parse_statement(const char *input, Statement *statement)
{
    if (input == NULL || statement == NULL) {
        return PARSER_SYNTAX_ERROR;
    }

    memset(statement, 0, sizeof(*statement));

    if (strcmp(input, ".exit") == 0) {
        statement->type = META_EXIT;
        return PARSER_OK;
    }

    if (strcmp(input, ".help") == 0) {
        statement->type = META_HELP;
        return PARSER_OK;
    }

    if (strcmp(input, ".btree") == 0) {
        statement->type = META_BTREE;
        return PARSER_OK;
    }

    if (parse_insert(input, statement) || parse_legacy_insert(input, statement)) {
        return PARSER_OK;
    }

    if (strcmp(input, "SELECT * FROM users;") == 0 ||
        strcmp(input, "SELECT * FROM users") == 0 ||
        strcmp(input, "select * from users;") == 0 ||
        strcmp(input, "select * from users") == 0) {
        statement->type = STATEMENT_SELECT;
        return PARSER_OK;
    }

    return PARSER_SYNTAX_ERROR;
}

const char *parser_result_string(ParserResult result)
{
    switch (result) {
    case PARSER_OK:
        return "ok";
    case PARSER_SYNTAX_ERROR:
        return "syntax error";
    case PARSER_INVALID_ID:
        return "invalid id";
    default:
        return "unknown parser error";
    }
}
