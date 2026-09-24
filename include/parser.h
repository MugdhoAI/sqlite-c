#ifndef PARSER_H
#define PARSER_H

#include "sqlite.h"

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT,
    META_EXIT,
    META_HELP
} StatementType;

typedef enum {
    PARSER_OK = 0,
    PARSER_SYNTAX_ERROR,
    PARSER_INVALID_ID
} ParserResult;

typedef struct {
    StatementType type;
    Row row;
} Statement;

ParserResult parse_statement(const char *input, Statement *statement);
const char *parser_result_string(ParserResult result);

#endif
