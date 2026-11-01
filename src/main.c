/* main.c - command-line front-end for the hex dumper.
 *
 * Usage: hexdump [-s SKIP] [-n LENGTH] [-v] [FILE]
 *   -s SKIP     start dumping SKIP bytes into the input
 *   -n LENGTH   dump at most LENGTH bytes
 *   -v          verbose: do not collapse repeated lines with '*'
 *   FILE        input file, or '-'/omitted for standard input
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hexdump.h"

static void usage(FILE *out, const char *prog)
{
    fprintf(out, "usage: %s [-s SKIP] [-n LENGTH] [-v] [FILE]\n", prog);
}

static int parse_ll(const char *s, long long *out)
{
    errno = 0;
    char *end = NULL;
    long long v = strtoll(s, &end, 0); /* 0 = accept 0x.. and decimal */
    if (errno != 0 || end == s || *end != '\0' || v < 0) {
        return -1;
    }
    *out = v;
    return 0;
}

int main(int argc, char **argv)
{
    hexdump_options opts = {.start_offset = 0, .max_bytes = -1, .squeeze = 1};
    long long skip = 0;
    const char *path = NULL;

    int i = 1;
    for (; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            usage(stdout, argv[0]);
            return 0;
        } else if (strcmp(arg, "-v") == 0) {
            opts.squeeze = 0;
        } else if (strcmp(arg, "-s") == 0 || strcmp(arg, "-n") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "%s: %s needs a value\n", argv[0], arg);
                return 2;
            }
            long long val;
            if (parse_ll(argv[++i], &val) != 0) {
                fprintf(stderr, "%s: invalid number for %s: %s\n", argv[0], arg, argv[i]);
                return 2;
            }
            if (strcmp(arg, "-s") == 0) {
                skip = val;
            } else {
                opts.max_bytes = val;
            }
        } else if (strcmp(arg, "-") == 0) {
            path = NULL;
            break;
        } else if (arg[0] == '-' && arg[1] != '\0') {
            fprintf(stderr, "%s: unknown option %s\n", argv[0], arg);
            usage(stderr, argv[0]);
            return 2;
        } else {
            path = arg;
            break;
        }
    }

    FILE *in = stdin;
    if (path != NULL) {
        in = fopen(path, "rb");
        if (in == NULL) {
            fprintf(stderr, "%s: cannot open %s: %s\n", argv[0], path, strerror(errno));
            return 1;
        }
    }

    if (skip > 0) {
        /* Try a cheap seek first; fall back to reading and discarding so that
         * pipes (which cannot seek) still honour -s. */
        if (fseek(in, skip, SEEK_SET) != 0) {
            char junk[4096];
            long long left = skip;
            while (left > 0) {
                size_t want = left < (long long)sizeof(junk) ? (size_t)left : sizeof(junk);
                size_t got = fread(junk, 1, want, in);
                if (got == 0) {
                    break;
                }
                left -= (long long)got;
            }
        }
        opts.start_offset = (uint64_t)skip;
    }

    int rc = hexdump_stream(in, stdout, &opts);
    if (rc != 0) {
        fprintf(stderr, "%s: read error\n", argv[0]);
    }
    if (in != stdin) {
        fclose(in);
    }
    return rc;
}
