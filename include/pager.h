#ifndef PAGER_H
#define PAGER_H

#include <stddef.h>
#include <stdio.h>

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100

typedef enum {
    PAGER_OK = 0,
    PAGER_INVALID_ARGUMENT,
    PAGER_IO_ERROR,
    PAGER_OUT_OF_PAGES
} PagerResult;

typedef struct {
    FILE *file;
    size_t file_length;
    size_t num_pages;
    void *pages[TABLE_MAX_PAGES];
} Pager;

Pager *pager_open(const char *filename, PagerResult *result);
void pager_close(Pager *pager);
void *pager_get_page(Pager *pager, size_t page_number, PagerResult *result);
PagerResult pager_flush(Pager *pager, size_t page_number);

const char *pager_result_string(PagerResult result);

#endif
