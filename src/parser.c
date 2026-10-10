#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static int valid_suffix(const char *suffix)
{
    while (isspace((unsigned char)*suffix)) {
        ++suffix;
    }

    if (*suffix == ';') {
        ++suffix;
    }

    while (isspace((unsigned char)*suffix)) {
        ++suffix;
    }

    return *suffix == '\0';
}

static int parse_insert(const char *input, Statement *statement)
{
    int consumed = 0;
    int matched = sscanf(
        input,
        "INSERT INTO users VALUES (%d, '%32[^']', '%255[^']')%n",
        &statement->row.id,
        statement->row.username,
        statement->row.email,
        &consumed
    );

    if (matched != 3) {
        consumed = 0;
        matched = sscanf(
            input,
            "insert into users values (%d, '%32[^']', '%255[^']')%n",
            &statement->row.id,
            statement->row.username,
            statement->row.email,
            &consumed
        );
    }

    if (matched == 3 && consumed > 0 && valid_suffix(input + consumed) &&
        statement->row.id > 0) {
        statement->type = STATEMENT_INSERT;
        return 1;
    }

    return 0;
}

static int parse_legacy_insert(const char *input, Statement *statement)
{
    int consumed = 0;
    int matched = sscanf(
        input,
        "insert %d %32s %255s%n",
        &statement->row.id,
        statement->row.username,
        statement->row.email,
        &consumed
    );

    if (matched == 3 && consumed > 0 && valid_suffix(input + consumed) &&
        statement->row.id > 0) {
        statement->type = STATEMENT_INSERT;
        return 1;
    }

    return 0;
}

static int parse_select(const char *input, Statement *statement)
{
    static const char *const select_all[] = {
        "SELECT * FROM users",
        "select * from users"
    };

    for (size_t i = 0; i < sizeof(select_all) / sizeof(select_all[0]); ++i) {
        size_t length = strlen(select_all[i]);
        if (strncmp(input, select_all[i], length) == 0 &&
            valid_suffix(input + length)) {
            statement->type = STATEMENT_SELECT;
            statement->where_id = 0;
            return 1;
        }
    }

    int consumed = 0;
    int matched = sscanf(input, "SELECT * FROM users WHERE id = %d%n",
                         &statement->where_id, &consumed);

    if (matched != 1 || consumed <= 0 || !valid_suffix(input + consumed)) {
        consumed = 0;
        matched = sscanf(input, "select * from users where id = %d%n",
                         &statement->where_id, &consumed);
    }

    if (matched == 1 && consumed > 0 && valid_suffix(input + consumed) &&
        statement->where_id > 0) {
        statement->type = STATEMENT_SELECT;
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

    if (parse_select(input, statement)) {
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
