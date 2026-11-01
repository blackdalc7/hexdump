/* hexdump.h - canonical hex + ASCII dump, in the style of `hexdump -C`. */
#ifndef HEXDUMP_H
#define HEXDUMP_H

#include <stdint.h>
#include <stdio.h>

#define HEXDUMP_BYTES_PER_LINE 16

/* Each formatted line fits comfortably in this buffer (offset + hex + ASCII). */
#define HEXDUMP_LINE_BUFSZ 128

/* Format one line for up to HEXDUMP_BYTES_PER_LINE bytes.
 *
 * Writes a NUL-terminated canonical line (no trailing newline) into out, e.g.
 *   "00000000  7f 45 4c 46 02 01 01 00  00 ...  |.ELF............|"
 * Short final lines are padded so the ASCII column stays aligned.
 *
 * offset is the file offset of data[0]; len must be 0..HEXDUMP_BYTES_PER_LINE.
 */
void hexdump_format_line(char *out, size_t outsz, uint64_t offset,
                         const uint8_t *data, size_t len);

/* Options for a streaming dump. */
typedef struct {
    uint64_t start_offset; /* value printed for the first byte */
    long long max_bytes;   /* -1 for "until EOF" */
    int squeeze;           /* collapse runs of identical lines with '*' */
} hexdump_options;

/* Dump everything from in to out. Returns 0 on success, non-zero on read
 * error. The final offset is printed on its own line, as `hexdump -C` does. */
int hexdump_stream(FILE *in, FILE *out, const hexdump_options *opts);

#endif /* HEXDUMP_H */
