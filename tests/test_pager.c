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

int main(void)
{
    test_pager_round_trip();
    test_partial_page_files_are_rejected();
    return 0;
}
