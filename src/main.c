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
    puts("insert <id> <username> <email>");
    puts("select");
}

static int parse_insert(const char *input, Row *row)
{
    char username[SQLITE_USERNAME_MAX + 1];
    char email[SQLITE_EMAIL_MAX + 1];

    if (sscanf(input, "insert %d %32s %255s", &row->id, username, email) != 3) {
        return 0;
    }

    strcpy(row->username, username);
    strcpy(row->email, email);

    return 1;
}

int main(void)
{
    Table *table = table_create(TABLE_CAPACITY);
    if (table == NULL) {
        fputs("failed to create table\n", stderr);
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

        if (strcmp(input, ".exit") == 0) {
            break;
        }

        if (strcmp(input, ".help") == 0) {
            print_help();
            continue;
        }

        if (strcmp(input, "select") == 0) {
            for (size_t i = 0; i < table_size(table); ++i) {
                const Row *row = table_row_at(table, i);
                printf("%d | %s | %s\n", row->id, row->username, row->email);
            }
            continue;
        }

        if (strncmp(input, "insert ", 7) == 0) {
            Row row;
            if (!parse_insert(input, &row)) {
                puts("syntax error");
                continue;
            }

            SqliteResult result = table_insert(table, &row);
            if (result != SQLITE_OK) {
                printf("error: %s\n", sqlite_result_string(result));
            }
            continue;
        }

        puts("unrecognized command");
    }

    if (table_flush(table) != SQLITE_OK) {
        fputs("failed to flush database\\n", stderr);
        table_destroy(table);
        return 1;
    }

    table_destroy(table);
    return 0;
}
