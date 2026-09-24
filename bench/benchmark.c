#include "sqlite.h"

#include <stdio.h>
#include <time.h>

#define BENCHMARK_ROWS 1000

int main(void)
{
    const char *filename = "build/benchmark.db";
    remove(filename);

    SqliteResult result;
    Table *table = table_open(filename, &result);
    if (table == NULL) {
        fprintf(stderr, "failed to open benchmark database: %s\n",
                sqlite_result_string(result));
        return 1;
    }

    clock_t start = clock();

    for (int id = 1; id <= BENCHMARK_ROWS; ++id) {
        Row row = {0};
        row.id = id;
        snprintf(row.username, sizeof(row.username), "user%d", id);
        snprintf(row.email, sizeof(row.email), "user%d@example.com", id);

        SqliteResult insert_result = table_insert(table, &row);
        if (insert_result != SQLITE_OK) {
            fprintf(stderr, "benchmark insert failed at row %d: %s\n",
                    id, sqlite_result_string(insert_result));
            table_destroy(table);
            remove(filename);
            return 1;
        }
    }

    if (table_flush(table) != SQLITE_OK) {
        fputs("benchmark flush failed\n", stderr);
        table_destroy(table);
        remove(filename);
        return 1;
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / (double)CLOCKS_PER_SEC;

    printf("inserted %d rows in %.6f seconds\n", BENCHMARK_ROWS, elapsed);

    table_destroy(table);
    remove(filename);
    return 0;
}
