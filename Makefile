CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Iinclude
BUILD_DIR := build
TARGET := $(BUILD_DIR)/sentineldtl
SOURCES := $(sort $(wildcard src/*.c))

.PHONY: all clean test ci list

all: $(TARGET)

$(TARGET): $(SOURCES) $(wildcard include/sentinel/*.h)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

test: $(TARGET)
	node --test "tests/node/*.test.js"

ci:
	bash scripts/ci.sh

list: $(TARGET)
	$(TARGET) --list

clean:
	rm -rf $(BUILD_DIR)
