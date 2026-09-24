CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -O2
CPPFLAGS ?= -Iinclude

BUILD_DIR := build
SRC_DIR := src
TEST_DIR := tests

SOURCES := $(SRC_DIR)/main.c $(SRC_DIR)/database.c $(SRC_DIR)/pager.c
OBJECTS := $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET := $(BUILD_DIR)/sqlite-c
TEST_TARGET := $(BUILD_DIR)/test_database

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(TEST_TARGET): $(TEST_DIR)/test_database.c $(SRC_DIR)/database.c $(SRC_DIR)/btree.c $(SRC_DIR)/pager.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@

$(BUILD_DIR)/test_pager: $(TEST_DIR)/test_pager.c $(SRC_DIR)/pager.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@

test: $(TEST_TARGET) $(BUILD_DIR)/test_pager
	./$(TEST_TARGET)
	./$(BUILD_DIR)/test_pager

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
