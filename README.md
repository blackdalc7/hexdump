# hexdump

A small `hexdump -C` clone written in C: it prints a file as offset + hex +
ASCII, the format you reach for whenever you need to see what is *actually* in a
binary. Written to understand how that classic layout is produced, and to
practise clean C with real tests.

```text
00000000  7f 45 4c 46 02 01 01 00  00 00 00 00 00 00 00 00  |.ELF............|
00000010  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00000040  68 65 78 64 75 6d 70 20  64 65 6d 6f 20 66 69 6c  |hexdump demo fil|
00000050  65 0a de ad be ef                                 |e.....|
00000056
```

## Features

- Canonical 16-bytes-per-line layout with an 8-digit offset and an ASCII gutter.
- **Repeat squeezing:** runs of identical lines collapse to a single `*`, just
  like `hexdump -C`. Turn it off with `-v`.
- Reads a file or standard input, so it works in a pipe.
- `-s` to skip into the input and `-n` to limit how much is shown. Skipping
  works even on non-seekable pipes (it reads and discards).

## Build and run

```bash
make                     # -> build/hexdump

./build/hexdump file.bin             # dump a file
./build/hexdump -n 64 file.bin       # first 64 bytes
./build/hexdump -s 0x200 file.bin    # start at offset 512
some_command | ./build/hexdump       # from a pipe
./build/hexdump -v file.bin          # do not collapse repeated lines
```

## Options

| Option | Meaning |
|---|---|
| `-s SKIP` | Start dumping `SKIP` bytes in (decimal or `0x` hex) |
| `-n LENGTH` | Dump at most `LENGTH` bytes |
| `-v` | Verbose: print every line, no `*` squeezing |
| `FILE` | Input file; `-` or omitted means standard input |

## Layout

```text
src/
    hexdump.h    public interface: format one line, stream a whole dump
    hexdump.c    the formatter + the streaming loop (offset, squeeze, EOF)
    main.c       argument parsing and file/stdin handling
tests/
    test_hexdump.c   unit tests (formatter + streaming, in-memory)
    golden.sh        runs the binary on a fixture and diffs the output
    golden/          input.bin + expected output (squeezed and verbose)
```

The formatting and the dump loop live in `hexdump.c` behind a small header, so
they can be tested directly. `main.c` only deals with the command line.

## Tests

```bash
make test
```

Two layers:

- **Unit tests** build `hexdump.c` into a test program and drive the streaming
  code with `fmemopen` / `open_memstream`, asserting on exact output for a full
  line, a short final line, non-printable bytes, the `*` squeeze, and `-s`/`-n`.
- **Golden test** runs the built binary against a committed fixture and diffs
  the result, so any change to the output format has to be deliberate.

CI builds and runs both on Linux and macOS.

## Licence

MIT. See [LICENSE](LICENSE).
