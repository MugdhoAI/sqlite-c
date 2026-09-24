#include "pager.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

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
    return 0;
}
