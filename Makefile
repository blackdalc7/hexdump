# Makefile for hexdump.
#
#   make         build build/hexdump
#   make test    unit tests + a golden-output comparison
#   make clean   remove build artefacts

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -O2 -Isrc
BUILD := build

.PHONY: all test clean

all: $(BUILD)/hexdump

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/hexdump: src/main.c src/hexdump.c src/hexdump.h | $(BUILD)
	$(CC) $(CFLAGS) src/main.c src/hexdump.c -o $@

$(BUILD)/test_hexdump: tests/test_hexdump.c src/hexdump.c src/hexdump.h | $(BUILD)
	$(CC) $(CFLAGS) tests/test_hexdump.c src/hexdump.c -o $@

test: $(BUILD)/test_hexdump $(BUILD)/hexdump
	$(BUILD)/test_hexdump
	./tests/golden.sh $(BUILD)/hexdump

clean:
	rm -rf $(BUILD)
