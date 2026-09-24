#include "parser.h"
#include "sqlite.h"

#include <stdio.h>
#include <string.h>

#define DATABASE_FILE "build/sqlite.db"

static void print_prompt(void)
{
    printf("db > ");
    fflush(stdout);
}

static void print_help(void)
{
    puts(".help");
    puts(".exit");
    puts("INSERT INTO users VALUES (id, 'username', 'email');");
    puts("SELECT * FROM users;");
}

static void print_rows(const Table *table)
{
    for (size_t i = 0; i < table_size(table); ++i) {
        const Row *row = table_row_at(table, i);
        if (row != NULL) {
            printf("%d | %s | %s\n", row->id, row->username, row->email);
        }
    }
}

int main(void)
{
    SqliteResult open_result;
    Table *table = table_open(DATABASE_FILE, &open_result);
    if (table == NULL) {
        fprintf(stderr, "failed to open database: %s\n",
                sqlite_result_string(open_result));
        return 1;
    }

    char input[512];

    for (;;) {
        print_prompt();

        if (fgets(input, sizeof(input), stdin) == NULL) {
            putchar('\n');
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        Statement statement;
        ParserResult parse_result = parse_statement(input, &statement);

        if (parse_result != PARSER_OK) {
            puts(parser_result_string(parse_result));
            continue;
        }

        switch (statement.type) {
        case META_EXIT:
            goto done;
        case META_HELP:
            print_help();
            break;
        case STATEMENT_SELECT:
            print_rows(table);
            break;
        case STATEMENT_INSERT: {
            SqliteResult result = table_insert(table, &statement.row);
            if (result != SQLITE_OK) {
                printf("error: %s\n", sqlite_result_string(result));
            }
            break;
        }
        }
    }

done:
    if (table_flush(table) != SQLITE_OK) {
        fputs("failed to flush database\n", stderr);
        table_destroy(table);
        return 1;
    }

    table_destroy(table);
    return 0;
}
