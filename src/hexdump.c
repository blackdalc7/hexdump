/* hexdump.c - implementation of the canonical dump. */
#include "hexdump.h"

#include <string.h>

static char ascii_of(uint8_t b)
{
    return (b >= 0x20 && b <= 0x7E) ? (char)b : '.';
}

void hexdump_format_line(char *out, size_t outsz, uint64_t offset,
                         const uint8_t *data, size_t len)
{
    if (len > HEXDUMP_BYTES_PER_LINE) {
        len = HEXDUMP_BYTES_PER_LINE;
    }

    char hex[64];
    char asc[HEXDUMP_BYTES_PER_LINE + 1];
    size_t h = 0;

    for (size_t i = 0; i < HEXDUMP_BYTES_PER_LINE; i++) {
        /* An extra space between the two 8-byte groups. */
        if (i == 8) {
            hex[h++] = ' ';
        }
        if (i < len) {
            static const char d[] = "0123456789abcdef";
            hex[h++] = d[data[i] >> 4];
            hex[h++] = d[data[i] & 0x0F];
            asc[i] = ascii_of(data[i]);
        } else {
            hex[h++] = ' ';
            hex[h++] = ' ';
            asc[i] = ' ';
        }
        hex[h++] = ' ';
    }
    hex[h] = '\0';

    /* The ASCII gutter shows only the real bytes, so a short final line keeps
     * the bars tight around its content (as `hexdump -C` does). */
    char tight[HEXDUMP_BYTES_PER_LINE + 1];
    memcpy(tight, asc, len);
    tight[len] = '\0';

    /* hex already ends with one space after the last byte column; one more
     * gives the canonical two-space gap before the ASCII gutter. */
    snprintf(out, outsz, "%08llx  %s |%s|",
             (unsigned long long)offset, hex, tight);
}

int hexdump_stream(FILE *in, FILE *out, const hexdump_options *opts)
{
    uint8_t buf[HEXDUMP_BYTES_PER_LINE];
    uint8_t prev[HEXDUMP_BYTES_PER_LINE];
    char line[HEXDUMP_LINE_BUFSZ];

    uint64_t offset = opts->start_offset;
    long long remaining = opts->max_bytes;
    int have_prev = 0;
    int squeezing = 0;

    while (remaining != 0) {
        size_t want = HEXDUMP_BYTES_PER_LINE;
        if (remaining > 0 && (long long)want > remaining) {
            want = (size_t)remaining;
        }
        size_t got = fread(buf, 1, want, in);
        if (got == 0) {
            if (ferror(in)) {
                return 1;
            }
            break; /* EOF */
        }

        int is_full = (got == HEXDUMP_BYTES_PER_LINE);
        if (opts->squeeze && is_full && have_prev &&
            memcmp(buf, prev, HEXDUMP_BYTES_PER_LINE) == 0) {
            if (!squeezing) {
                fputs("*\n", out);
                squeezing = 1;
            }
        } else {
            hexdump_format_line(line, sizeof(line), offset, buf, got);
            fputs(line, out);
            fputc('\n', out);
            squeezing = 0;
        }

        memcpy(prev, buf, got);
        have_prev = is_full;
        offset += got;
        if (remaining > 0) {
            remaining -= (long long)got;
        }
    }

    /* Final offset line, matching `hexdump -C`. */
    fprintf(out, "%08llx\n", (unsigned long long)offset);
    return 0;
}
