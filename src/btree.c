#include "btree.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
    NODE_LEAF = 1,
    NODE_INTERNAL = 2
};

#define NODE_HEADER_SIZE 12U
#define LEAF_CELL_SIZE SQLITE_ROW_SIZE
#define LEAF_MAX_CELLS ((PAGE_SIZE - NODE_HEADER_SIZE) / LEAF_CELL_SIZE)
#define INTERNAL_CELL_SIZE 8U
#define INTERNAL_MAX_CELLS ((PAGE_SIZE - NODE_HEADER_SIZE) / INTERNAL_CELL_SIZE)
#define LEAF_SPLIT_LEFT ((LEAF_MAX_CELLS + 1U) / 2U)
#define LEAF_SPLIT_RIGHT ((LEAF_MAX_CELLS + 1U) - LEAF_SPLIT_LEFT)

#define META_MAGIC 0x53514c43U
#define META_VERSION 1U

struct BTree {
    Pager *pager;
    uint32_t root_page;
    uint32_t row_count;
};

static uint32_t read_u32(const unsigned char *source)
{
    return ((uint32_t)source[0]) |
           ((uint32_t)source[1] << 8U) |
           ((uint32_t)source[2] << 16U) |
           ((uint32_t)source[3] << 24U);
}

static void write_u32(unsigned char *destination, uint32_t value)
{
    destination[0] = (unsigned char)(value & 0xffU);
    destination[1] = (unsigned char)((value >> 8U) & 0xffU);
    destination[2] = (unsigned char)((value >> 16U) & 0xffU);
    destination[3] = (unsigned char)((value >> 24U) & 0xffU);
}

static void serialize_row(unsigned char *destination, const Row *row)
{
    write_u32(destination, (uint32_t)row->id);
    memcpy(destination + sizeof(uint32_t), row->username, SQLITE_USERNAME_MAX + 1U);
    memcpy(destination + sizeof(uint32_t) + SQLITE_USERNAME_MAX + 1U,
           row->email, SQLITE_EMAIL_MAX + 1U);
}

static void deserialize_row(Row *row, const unsigned char *source)
{
    row->id = (int)read_u32(source);
    memcpy(row->username, source + sizeof(uint32_t), SQLITE_USERNAME_MAX + 1U);
    memcpy(row->email,
           source + sizeof(uint32_t) + SQLITE_USERNAME_MAX + 1U,
           SQLITE_EMAIL_MAX + 1U);
    row->username[SQLITE_USERNAME_MAX] = '\0';
    row->email[SQLITE_EMAIL_MAX] = '\0';
}

static unsigned char *page_for(BTree *tree, uint32_t page_number, BTreeResult *result)
{
    PagerResult pager_result;
    unsigned char *page = pager_get_page(tree->pager, page_number, &pager_result);
    if (page == NULL) {
        *result = PAGER_OUT_OF_PAGES == pager_result ? BTREE_TABLE_FULL : BTREE_IO_ERROR;
        return NULL;
    }

    return page;
}

static const unsigned char *const_page_for(const BTree *tree, uint32_t page_number,
                                           BTreeResult *result)
{
    return page_for((BTree *)tree, page_number, result);
}

static uint8_t node_type(const unsigned char *page)
{
    return page[0];
}

static void set_node_type(unsigned char *page, uint8_t type)
{
    page[0] = type;
}

static uint32_t node_parent(const unsigned char *page)
{
    return read_u32(page + 4U);
}

static void set_node_parent(unsigned char *page, uint32_t parent)
{
    write_u32(page + 4U, parent);
}

static uint32_t node_count(const unsigned char *page)
{
    return read_u32(page + 8U);
}

static void set_node_count(unsigned char *page, uint32_t count)
{
    write_u32(page + 8U, count);
}

static uint32_t leaf_next(const unsigned char *page)
{
    return read_u32(page + 12U);
}

static void set_leaf_next(unsigned char *page, uint32_t next)
{
    write_u32(page + 12U, next);
}

static uint32_t leaf_header_size(void)
{
    return 16U;
}

static uint32_t internal_header_size(void)
{
    return 16U;
}

static unsigned char *leaf_cell(unsigned char *page, uint32_t index)
{
    return page + leaf_header_size() + ((size_t)index * LEAF_CELL_SIZE);
}

static const unsigned char *const_leaf_cell(const unsigned char *page, uint32_t index)
{
    return page + leaf_header_size() + ((size_t)index * LEAF_CELL_SIZE);
}

static uint32_t leaf_key(const unsigned char *page, uint32_t index)
{
    return read_u32(const_leaf_cell(page, index));
}

static unsigned char *internal_cell(unsigned char *page, uint32_t index)
{
    return page + internal_header_size() + ((size_t)index * INTERNAL_CELL_SIZE);
}

static const unsigned char *const_internal_cell(const unsigned char *page, uint32_t index)
{
    return page + internal_header_size() + ((size_t)index * INTERNAL_CELL_SIZE);
}

static uint32_t internal_child(const unsigned char *page, uint32_t index)
{
    return read_u32(const_internal_cell(page, index));
}

static uint32_t internal_key(const unsigned char *page, uint32_t index)
{
    return read_u32(const_internal_cell(page, index) + 4U);
}

static uint32_t internal_right_child(const unsigned char *page)
{
    return read_u32(page + 12U);
}

static void set_internal_right_child(unsigned char *page, uint32_t child)
{
    write_u32(page + 12U, child);
}

static void initialize_leaf(unsigned char *page, uint32_t parent)
{
    memset(page, 0, PAGE_SIZE);
    set_node_type(page, NODE_LEAF);
    set_node_parent(page, parent);
    set_node_count(page, 0);
    set_leaf_next(page, 0);
}

static void initialize_internal(unsigned char *page, uint32_t parent)
{
    memset(page, 0, PAGE_SIZE);
    set_node_type(page, NODE_INTERNAL);
    set_node_parent(page, parent);
    set_node_count(page, 0);
    set_internal_right_child(page, 0);
}

static uint32_t allocate_page(BTree *tree, BTreeResult *result)
{
    uint32_t page_number = (uint32_t)tree->pager->num_pages;
    if (page_number >= TABLE_MAX_PAGES) {
        *result = BTREE_TABLE_FULL;
        return 0;
    }

    unsigned char *page = page_for(tree, page_number, result);
    if (page == NULL) {
        return 0;
    }

    memset(page, 0, PAGE_SIZE);
    return page_number;
}

static void write_metadata(BTree *tree)
{
    BTreeResult result;
    unsigned char *page = page_for(tree, 0, &result);
    if (page == NULL) {
        return;
    }

    write_u32(page, META_MAGIC);
    write_u32(page + 4U, META_VERSION);
    write_u32(page + 8U, tree->root_page);
    write_u32(page + 12U, tree->row_count);
    (void)pager_flush(tree->pager, 0);
}

static uint32_t leaf_find_index(const unsigned char *page, int id)
{
    uint32_t low = 0;
    uint32_t high = node_count(page);

    while (low < high) {
        uint32_t middle = low + ((high - low) / 2U);
        uint32_t key = leaf_key(page, middle);

        if (key < (uint32_t)id) {
            low = middle + 1U;
        } else {
            high = middle;
        }
    }

    return low;
}

static uint32_t internal_child_index(const unsigned char *page, int id)
{
    uint32_t count = node_count(page);

    for (uint32_t i = 0; i < count; ++i) {
        if ((uint32_t)id < internal_key(page, i)) {
            return i;
        }
    }

    return count;
}

static uint32_t child_page_at(const unsigned char *page, uint32_t index)
{
    return index == node_count(page) ? internal_right_child(page) : internal_child(page, index);
}

static uint32_t max_key(const BTree *tree, uint32_t page_number, BTreeResult *result)
{
    const unsigned char *page = const_page_for(tree, page_number, result);
    if (page == NULL) {
        return 0;
    }

    if (node_type(page) == NODE_LEAF) {
        uint32_t count = node_count(page);
        return count == 0 ? 0 : leaf_key(page, count - 1U);
    }

    return max_key(tree, internal_right_child(page), result);
}

static BTreeResult flush_page(BTree *tree, uint32_t page_number)
{
    return pager_flush(tree->pager, page_number) == PAGER_OK ? BTREE_OK : BTREE_IO_ERROR;
}

static BTreeResult update_parent_separator(BTree *tree, uint32_t leaf_page)
{
    BTreeResult result;
    unsigned char *leaf = page_for(tree, leaf_page, &result);
    if (leaf == NULL) {
        return result;
    }

    uint32_t parent_page = node_parent(leaf);
    if (parent_page == 0) {
        return BTREE_OK;
    }

    unsigned char *parent = page_for(tree, parent_page, &result);
    if (parent == NULL) {
        return result;
    }

    uint32_t count = node_count(parent);
    for (uint32_t i = 0; i < count; ++i) {
        if (internal_child(parent, i) == leaf_page) {
            write_u32(internal_cell(parent, i) + 4U, max_key(tree, leaf_page, &result));
            if (result != BTREE_OK) {
                return result;
            }
            return flush_page(tree, parent_page);
        }
    }

    return BTREE_OK;
}

static BTreeResult internal_insert(BTree *tree, uint32_t parent_page,
                                   uint32_t left_child, uint32_t right_child)
{
    BTreeResult result;
    unsigned char *parent = page_for(tree, parent_page, &result);
    if (parent == NULL) {
        return result;
    }

    uint32_t count = node_count(parent);
    if (count >= INTERNAL_MAX_CELLS) {
        return BTREE_TABLE_FULL;
    }

    uint32_t insert_at = 0;
    while (insert_at < count && internal_child(parent, insert_at) != left_child) {
        ++insert_at;
    }

    if (insert_at == count && internal_right_child(parent) != left_child) {
        return BTREE_CORRUPT;
    }

    uint32_t separator = max_key(tree, left_child, &result);
    if (result != BTREE_OK) {
        return result;
    }

    if (insert_at == count) {
        set_internal_right_child(parent, right_child);
        unsigned char *cell = internal_cell(parent, count);
        write_u32(cell, left_child);
        write_u32(cell + 4U, separator);
    } else {
        uint32_t old_right = internal_child(parent, insert_at);
        for (uint32_t i = count; i > insert_at; --i) {
            memcpy(internal_cell(parent, i), internal_cell(parent, i - 1U),
                   INTERNAL_CELL_SIZE);
        }
        write_u32(internal_cell(parent, insert_at), left_child);
        write_u32(internal_cell(parent, insert_at) + 4U, separator);
        set_internal_right_child(parent, old_right);
        for (uint32_t i = insert_at + 1U; i < count + 1U; ++i) {
            if (i < count) {
                uint32_t child = internal_child(parent, i);
                unsigned char *child_page_data = page_for(tree, child, &result);
                if (child_page_data == NULL) {
                    return result;
                }
                set_node_parent(child_page_data, parent_page);
            }
        }
    }

    set_node_count(parent, count + 1U);

    unsigned char *right = page_for(tree, right_child, &result);
    if (right == NULL) {
        return result;
    }
    set_node_parent(right, parent_page);

    return flush_page(tree, parent_page);
}

static BTreeResult create_root_from_split(BTree *tree,
                                          uint32_t old_root,
                                          uint32_t right_child)
{
    BTreeResult result;
    unsigned char *root = page_for(tree, old_root, &result);
    if (root == NULL) {
        return result;
    }

    uint32_t left_child = allocate_page(tree, &result);
    if (result != BTREE_OK) {
        return result;
    }

    unsigned char *left = page_for(tree, left_child, &result);
    if (left == NULL) {
        return result;
    }

    memcpy(left, root, PAGE_SIZE);
    set_node_parent(left, old_root);

    unsigned char *right = page_for(tree, right_child, &result);
    if (right == NULL) {
        return result;
    }

    set_node_parent(right, old_root);
    set_node_type(root, NODE_INTERNAL);
    set_node_parent(root, 0);
    set_node_count(root, 1);
    write_u32(internal_cell(root, 0), left_child);
    write_u32(internal_cell(root, 0) + 4U, max_key(tree, left_child, &result));
    set_internal_right_child(root, right_child);

    if (result != BTREE_OK) {
        return result;
    }

    return flush_page(tree, old_root);
}

static BTreeResult split_leaf(BTree *tree, uint32_t leaf_page,
                              int id, const Row *row)
{
    BTreeResult result;
    unsigned char *old_page = page_for(tree, leaf_page, &result);
    if (old_page == NULL) {
        return result;
    }

    uint32_t new_page_number = allocate_page(tree, &result);
    if (result != BTREE_OK) {
        return result;
    }

    unsigned char *new_page = page_for(tree, new_page_number, &result);
    if (new_page == NULL) {
        return result;
    }

    uint32_t old_count = node_count(old_page);
    Row combined[LEAF_MAX_CELLS + 1U];
    uint32_t insert_at = leaf_find_index(old_page, id);

    for (uint32_t i = 0; i < old_count + 1U; ++i) {
        if (i == insert_at) {
            combined[i] = *row;
        } else {
            uint32_t source = i > insert_at ? i - 1U : i;
            deserialize_row(&combined[i], const_leaf_cell(old_page, source));
        }
    }

    uint32_t parent = node_parent(old_page);
    uint32_t next = leaf_next(old_page);

    initialize_leaf(old_page, parent);
    initialize_leaf(new_page, parent);

    for (uint32_t i = 0; i < LEAF_SPLIT_LEFT; ++i) {
        unsigned char *cell = leaf_cell(old_page, i);
        serialize_row(cell, &combined[i]);
    }
    set_node_count(old_page, LEAF_SPLIT_LEFT);
    set_leaf_next(old_page, new_page_number);

    for (uint32_t i = 0; i < LEAF_SPLIT_RIGHT; ++i) {
        unsigned char *cell = leaf_cell(new_page, i);
        serialize_row(cell, &combined[LEAF_SPLIT_LEFT + i]);
    }
    set_node_count(new_page, LEAF_SPLIT_RIGHT);
    set_leaf_next(new_page, next);

    if (parent == 0) {
        result = create_root_from_split(tree, leaf_page, new_page_number);
    } else {
        result = internal_insert(tree, parent, leaf_page, new_page_number);
    }

    if (result != BTREE_OK) {
        return result;
    }

    result = flush_page(tree, leaf_page);
    if (result != BTREE_OK) {
        return result;
    }

    return flush_page(tree, new_page_number);
}

BTree *btree_open(Pager *pager, BTreeResult *result)
{
    if (pager == NULL || result == NULL) {
        return NULL;
    }

    *result = BTREE_OK;

    BTree *tree = calloc(1, sizeof(*tree));
    if (tree == NULL) {
        *result = BTREE_IO_ERROR;
        return NULL;
    }

    tree->pager = pager;

    PagerResult pager_result;
    unsigned char *meta = pager_get_page(pager, 0, &pager_result);
    if (meta == NULL) {
        free(tree);
        *result = BTREE_IO_ERROR;
        return NULL;
    }

    if (pager->num_pages == 1U && read_u32(meta) == 0U) {
        tree->root_page = 1;
        tree->row_count = 0;

        unsigned char *root = page_for(tree, tree->root_page, result);
        if (root == NULL) {
            free(tree);
            return NULL;
        }

        initialize_leaf(root, 0);
        write_metadata(tree);
        return tree;
    }

    if (read_u32(meta) != META_MAGIC || read_u32(meta + 4U) != META_VERSION) {
        free(tree);
        *result = BTREE_CORRUPT;
        return NULL;
    }

    tree->root_page = read_u32(meta + 8U);
    tree->row_count = read_u32(meta + 12U);

    if (tree->root_page == 0 || tree->root_page >= TABLE_MAX_PAGES ||
        tree->root_page >= pager->num_pages) {
        free(tree);
        *result = BTREE_CORRUPT;
        return NULL;
    }

    return tree;
}

BTreeResult btree_flush(BTree *tree)
{
    if (tree == NULL) {
        return BTREE_INVALID_ARGUMENT;
    }

    write_metadata(tree);

    for (size_t page_number = 0; page_number < tree->pager->num_pages; ++page_number) {
        if (tree->pager->pages[page_number] != NULL &&
            pager_flush(tree->pager, page_number) != PAGER_OK) {
            return BTREE_IO_ERROR;
        }
    }

    return BTREE_OK;
}

void btree_close(BTree *tree)
{
    if (tree == NULL) {
        return;
    }

    (void)btree_flush(tree);
    free(tree);
}

BTreeResult btree_insert(BTree *tree, const Row *row)
{
    if (tree == NULL || row == NULL || row->id <= 0) {
        return BTREE_INVALID_ARGUMENT;
    }

    BTreeResult result;
    uint32_t page_number = tree->root_page;

    for (;;) {
        unsigned char *page = page_for(tree, page_number, &result);
        if (page == NULL) {
            return result;
        }

        if (node_type(page) == NODE_LEAF) {
            uint32_t count = node_count(page);
            uint32_t index = leaf_find_index(page, row->id);

            if (index < count && leaf_key(page, index) == (uint32_t)row->id) {
                return BTREE_DUPLICATE_KEY;
            }

            if (count >= LEAF_MAX_CELLS) {
                result = split_leaf(tree, page_number, row->id, row);
                if (result == BTREE_OK) {
                    ++tree->row_count;
                    write_metadata(tree);
                }
                return result;
            }

            for (uint32_t i = count; i > index; --i) {
                memcpy(leaf_cell(page, i), leaf_cell(page, i - 1U), LEAF_CELL_SIZE);
            }

            serialize_row(leaf_cell(page, index), row);
            set_node_count(page, count + 1U);
            result = flush_page(tree, page_number);
            if (result != BTREE_OK) {
                return result;
            }

            result = update_parent_separator(tree, page_number);
            if (result != BTREE_OK) {
                return result;
            }

            ++tree->row_count;
            write_metadata(tree);
            return BTREE_OK;
        }

        uint32_t index = internal_child_index(page, row->id);
        page_number = child_page_at(page, index);
    }
}

BTreeResult btree_find(const BTree *tree, int id, Row *row)
{
    if (tree == NULL || row == NULL || id <= 0) {
        return BTREE_INVALID_ARGUMENT;
    }

    BTreeResult result;
    uint32_t page_number = tree->root_page;

    for (;;) {
        const unsigned char *page = const_page_for(tree, page_number, &result);
        if (page == NULL) {
            return result;
        }

        if (node_type(page) == NODE_LEAF) {
            uint32_t count = node_count(page);
            uint32_t index = leaf_find_index(page, id);
            if (index < count && leaf_key(page, index) == (uint32_t)id) {
                deserialize_row(row, const_leaf_cell(page, index));
                return BTREE_OK;
            }
            return BTREE_INVALID_ARGUMENT;
        }

        page_number = child_page_at(page, internal_child_index(page, id));
    }
}

BTreeResult btree_read_all(const BTree *tree, Row *rows, size_t capacity, size_t *count)
{
    if (tree == NULL || rows == NULL || count == NULL) {
        return BTREE_INVALID_ARGUMENT;
    }

    *count = 0;
    BTreeResult result;
    uint32_t page_number = tree->root_page;

    const unsigned char *page = const_page_for(tree, page_number, &result);
    if (page == NULL) {
        return result;
    }

    while (node_type(page) == NODE_INTERNAL) {
        page_number = child_page_at(page, 0);
        page = const_page_for(tree, page_number, &result);
        if (page == NULL) {
            return result;
        }
    }

    while (page_number != 0) {
        uint32_t cells = node_count(page);
        for (uint32_t i = 0; i < cells; ++i) {
            if (*count >= capacity) {
                return BTREE_TABLE_FULL;
            }
            deserialize_row(&rows[*count], const_leaf_cell(page, i));
            ++(*count);
        }

        page_number = leaf_next(page);
        if (page_number == 0) {
            break;
        }

        page = const_page_for(tree, page_number, &result);
        if (page == NULL) {
            return result;
        }
    }

    return BTREE_OK;
}

size_t btree_size(const BTree *tree)
{
    return tree == NULL ? 0U : tree->row_count;
}

const char *btree_result_string(BTreeResult result)
{
    switch (result) {
    case BTREE_OK:
        return "ok";
    case BTREE_INVALID_ARGUMENT:
        return "invalid argument";
    case BTREE_IO_ERROR:
        return "database I/O error";
    case BTREE_DUPLICATE_KEY:
        return "id already exists";
    case BTREE_TABLE_FULL:
        return "database capacity reached";
    case BTREE_CORRUPT:
        return "database file is corrupt";
    default:
        return "unknown error";
    }
}
