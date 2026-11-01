/* test_hexdump.c - unit tests for the formatter and the streaming dump.
 *
 * Uses fmemopen / open_memstream (POSIX) to run the real streaming code
 * against in-memory buffers, so the dump loop is tested end to end without
 * touching the filesystem.
 */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hexdump.h"

static void test_full_line(void)
{
    const uint8_t elf[16] = {0x7f, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    char line[HEXDUMP_LINE_BUFSZ];
    hexdump_format_line(line, sizeof(line), 0, elf, 16);
    const char *want =
        "00000000  7f 45 4c 46 02 01 01 00  00 00 00 00 00 00 00 00  |.ELF............|";
    assert(strcmp(line, want) == 0);
}

static void test_short_line_alignment(void)
{
    const uint8_t data[3] = {'h', 'i', '!'};
    char line[HEXDUMP_LINE_BUFSZ];
    hexdump_format_line(line, sizeof(line), 0x10, data, 3);
    /* Three bytes: hex columns padded, ASCII gutter holds only the 3 chars. */
    const char *want =
        "00000010  68 69 21                                          |hi!|";
    assert(strcmp(line, want) == 0);
}

static void test_non_printable_becomes_dot(void)
{
    const uint8_t data[4] = {0x00, 0x1f, 0x7f, 0x80};
    char line[HEXDUMP_LINE_BUFSZ];
    hexdump_format_line(line, sizeof(line), 0, data, 4);
    assert(strstr(line, "|....|") != NULL);
}

static char *dump_to_string(const uint8_t *data, size_t n, hexdump_options opts)
{
    FILE *in = fmemopen((void *)data, n, "rb");
    assert(in != NULL);
    char *out_buf = NULL;
    size_t out_sz = 0;
    FILE *out = open_memstream(&out_buf, &out_sz);
    assert(out != NULL);
    int rc = hexdump_stream(in, out, &opts);
    assert(rc == 0);
    fclose(out);
    fclose(in);
    return out_buf; /* caller frees */
}

static void test_stream_simple(void)
{
    const uint8_t data[3] = {'a', 'b', 'c'};
    hexdump_options opts = {.start_offset = 0, .max_bytes = -1, .squeeze = 1};
    char *out = dump_to_string(data, sizeof(data), opts);
    const char *want =
        "00000000  61 62 63                                          |abc|\n"
        "00000003\n";
    assert(strcmp(out, want) == 0);
    free(out);
}

static void test_stream_squeeze(void)
{
    /* 48 zero bytes = three identical full lines; squeeze -> line, '*', end. */
    uint8_t data[48];
    memset(data, 0, sizeof(data));
    hexdump_options opts = {.start_offset = 0, .max_bytes = -1, .squeeze = 1};
    char *out = dump_to_string(data, sizeof(data), opts);
    assert(strstr(out, "\n*\n") != NULL);
    /* The zero line appears once, then a single '*'. */
    const char *first = strstr(out, "00000000  00 00");
    assert(first != NULL);
    assert(strstr(out, "00000030\n") != NULL); /* 48 = 0x30 final offset */
    free(out);
}

static void test_stream_no_squeeze(void)
{
    uint8_t data[32];
    memset(data, 0, sizeof(data));
    hexdump_options opts = {.start_offset = 0, .max_bytes = -1, .squeeze = 0};
    char *out = dump_to_string(data, sizeof(data), opts);
    assert(strstr(out, "*") == NULL); /* every line printed */
    assert(strstr(out, "00000010  00 00") != NULL);
    free(out);
}

static void test_stream_max_and_offset(void)
{
    const uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    hexdump_options opts = {.start_offset = 0x100, .max_bytes = 4, .squeeze = 1};
    char *out = dump_to_string(data, sizeof(data), opts);
    assert(strncmp(out, "00000100  01 02 03 04", 20) == 0);
    assert(strstr(out, "00000104\n") != NULL); /* stopped after 4 bytes */
    free(out);
}

int main(void)
{
    test_full_line();
    test_short_line_alignment();
    test_non_printable_becomes_dot();
    test_stream_simple();
    test_stream_squeeze();
    test_stream_no_squeeze();
    test_stream_max_and_offset();
    printf("all hexdump tests passed\n");
    return 0;
}
