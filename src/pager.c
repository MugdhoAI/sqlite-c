#include "pager.h"

#include <stdlib.h>

static size_t page_count_for_length(size_t file_length)
{
    return (file_length + PAGE_SIZE - 1U) / PAGE_SIZE;
}

Pager *pager_open(const char *filename, PagerResult *result)
{
    if (filename == NULL || result == NULL) {
        return NULL;
    }

    *result = PAGER_OK;

    FILE *file = fopen(filename, "r+b");
    if (file == NULL) {
        file = fopen(filename, "w+b");
    }

    if (file == NULL) {
        *result = PAGER_IO_ERROR;
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        *result = PAGER_IO_ERROR;
        return NULL;
    }

    long length = ftell(file);
    if (length < 0) {
        fclose(file);
        *result = PAGER_IO_ERROR;
        return NULL;
    }

    Pager *pager = calloc(1, sizeof(*pager));
    if (pager == NULL) {
        fclose(file);
        *result = PAGER_IO_ERROR;
        return NULL;
    }

    pager->file = file;
    pager->file_length = (size_t)length;
    pager->num_pages = page_count_for_length(pager->file_length);

    if (pager->num_pages > TABLE_MAX_PAGES) {
        fclose(file);
        free(pager);
        *result = PAGER_OUT_OF_PAGES;
        return NULL;
    }

    return pager;
}

void pager_close(Pager *pager)
{
    if (pager == NULL) {
        return;
    }

    for (size_t page_number = 0; page_number < pager->num_pages; ++page_number) {
        if (pager->pages[page_number] != NULL) {
            (void)pager_flush(pager, page_number);
            free(pager->pages[page_number]);
        }
    }

    fclose(pager->file);
    free(pager);
}

void *pager_get_page(Pager *pager, size_t page_number, PagerResult *result)
{
    if (pager == NULL || result == NULL) {
        return NULL;
    }

    *result = PAGER_OK;

    if (page_number >= TABLE_MAX_PAGES) {
        *result = PAGER_OUT_OF_PAGES;
        return NULL;
    }

    if (pager->pages[page_number] == NULL) {
        void *page = calloc(1, PAGE_SIZE);
        if (page == NULL) {
            *result = PAGER_IO_ERROR;
            return NULL;
        }

        size_t pages_on_disk = page_count_for_length(pager->file_length);
        if (page_number < pages_on_disk) {
            if (fseek(pager->file, (long)(page_number * PAGE_SIZE), SEEK_SET) != 0 ||
                fread(page, PAGE_SIZE, 1, pager->file) != 1) {
                free(page);
                *result = PAGER_IO_ERROR;
                return NULL;
            }
        }

        pager->pages[page_number] = page;

        if (page_number >= pager->num_pages) {
            pager->num_pages = page_number + 1U;
        }
    }

    return pager->pages[page_number];
}

PagerResult pager_flush(Pager *pager, size_t page_number)
{
    if (pager == NULL || page_number >= pager->num_pages ||
        pager->pages[page_number] == NULL) {
        return PAGER_INVALID_ARGUMENT;
    }

    if (fseek(pager->file, (long)(page_number * PAGE_SIZE), SEEK_SET) != 0 ||
        fwrite(pager->pages[page_number], PAGE_SIZE, 1, pager->file) != 1 ||
        fflush(pager->file) != 0) {
        return PAGER_IO_ERROR;
    }

    size_t end = (page_number + 1U) * PAGE_SIZE;
    if (end > pager->file_length) {
        pager->file_length = end;
    }

    return PAGER_OK;
}

const char *pager_result_string(PagerResult result)
{
    switch (result) {
    case PAGER_OK:
        return "ok";
    case PAGER_INVALID_ARGUMENT:
        return "invalid argument";
    case PAGER_IO_ERROR:
        return "I/O error";
    case PAGER_OUT_OF_PAGES:
        return "database exceeds maximum page count";
    default:
        return "unknown error";
    }
}
