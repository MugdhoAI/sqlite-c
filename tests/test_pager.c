#include "pager.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void write_file_of_size(const char *filename, size_t size)
{
    unsigned char bytes[PAGE_SIZE + 1U] = {0};
    FILE *file = fopen(filename, "wb");
    assert(file != NULL);
    assert(size <= sizeof(bytes));
    assert(fwrite(bytes, 1U, size, file) == size);
    assert(fclose(file) == 0);
}

static void test_partial_page_files_are_rejected(void)
{
    const char *filename = "build/test_pager_partial.db";
    const size_t invalid_sizes[] = {1U, PAGE_SIZE - 1U, PAGE_SIZE + 1U};

    for (size_t i = 0; i < sizeof(invalid_sizes) / sizeof(invalid_sizes[0]); ++i) {
        write_file_of_size(filename, invalid_sizes[i]);

        PagerResult result = PAGER_OK;
        Pager *pager = pager_open(filename, &result);
        assert(pager == NULL);
        assert(result == PAGER_CORRUPT);
    }

    remove(filename);
}

static void test_pager_round_trip(void)
{
    const char *filename = "build/test_pager.db";

    remove(filename);

    PagerResult result;
    Pager *pager = pager_open(filename, &result);
    assert(pager != NULL);
    assert(result == PAGER_OK);

    unsigned char *page = pager_get_page(pager, 0, &result);
    assert(page != NULL);
    assert(result == PAGER_OK);

    const char marker[] = "sqlite-c page";
    memcpy(page, marker, sizeof(marker));

    assert(pager_flush(pager, 0) == PAGER_OK);
    pager_close(pager);

    pager = pager_open(filename, &result);
    assert(pager != NULL);

    page = pager_get_page(pager, 0, &result);
    assert(page != NULL);
    assert(memcmp(page, marker, sizeof(marker)) == 0);

    pager_close(pager);
    remove(filename);
}

static void test_pager_rejects_out_of_range_pages(void)
{
    const char *filename = "build/test_pager_range.db";

    remove(filename);

    PagerResult result;
    Pager *pager = pager_open(filename, &result);
    assert(pager != NULL);
    assert(result == PAGER_OK);

    unsigned char *page = pager_get_page(pager, TABLE_MAX_PAGES - 1U, &result);
    assert(page != NULL);
    assert(result == PAGER_OK);

    page = pager_get_page(pager, TABLE_MAX_PAGES, &result);
    assert(page == NULL);
    assert(result == PAGER_OUT_OF_PAGES);

    page = pager_get_page(pager, TABLE_MAX_PAGES + 10U, &result);
    assert(page == NULL);
    assert(result == PAGER_OUT_OF_PAGES);

    assert(pager_close(pager) == PAGER_OK);
    remove(filename);
}

static void test_pager_flush_rejects_invalid_arguments(void)
{
    const char *filename = "build/test_pager_flush.db";

    remove(filename);

    assert(pager_flush(NULL, 0) == PAGER_INVALID_ARGUMENT);
    assert(pager_close(NULL) == PAGER_INVALID_ARGUMENT);

    PagerResult result;
    Pager *pager = pager_open(filename, &result);
    assert(pager != NULL);
    assert(result == PAGER_OK);

    assert(pager_flush(pager, 0) == PAGER_INVALID_ARGUMENT);

    unsigned char *page = pager_get_page(pager, 0, &result);
    assert(page != NULL);
    assert(result == PAGER_OK);
    assert(pager_flush(pager, 0) == PAGER_OK);
    assert(pager_flush(pager, 1) == PAGER_INVALID_ARGUMENT);
    assert(pager_flush(pager, TABLE_MAX_PAGES) == PAGER_INVALID_ARGUMENT);

    assert(pager_close(pager) == PAGER_OK);
    remove(filename);
}

static void test_pager_open_rejects_null_arguments(void)
{
    PagerResult result = PAGER_OK;
    assert(pager_open(NULL, &result) == NULL);
    assert(pager_open("build/test_pager_null.db", NULL) == NULL);
    assert(pager_get_page(NULL, 0, &result) == NULL);
}

static void test_pager_open_rejects_file_exceeding_max_pages(void)
{
    const char *filename = "build/test_pager_too_large.db";
    unsigned char page[PAGE_SIZE] = {0};

    remove(filename);

    FILE *file = fopen(filename, "wb");
    assert(file != NULL);
    for (size_t i = 0; i < TABLE_MAX_PAGES + 1U; ++i) {
        assert(fwrite(page, sizeof(page), 1U, file) == 1U);
    }
    assert(fclose(file) == 0);

    PagerResult result = PAGER_OK;
    Pager *pager = pager_open(filename, &result);
    assert(pager == NULL);
    assert(result == PAGER_OUT_OF_PAGES);

    remove(filename);
}

int main(void)
{
    test_pager_round_trip();
    test_partial_page_files_are_rejected();
    test_pager_rejects_out_of_range_pages();
    test_pager_flush_rejects_invalid_arguments();
    test_pager_open_rejects_null_arguments();
    test_pager_open_rejects_file_exceeding_max_pages();
    return 0;
}
