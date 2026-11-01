#!/usr/bin/env bash
# Compare the hexdump binary's output against committed golden files.
set -euo pipefail
BIN="${1:-build/hexdump}"
here="$(cd "$(dirname "$0")" && pwd)"
fixture="$here/golden/input.bin"

fail=0
check() {
    local desc="$1" expected="$2"; shift 2
    if diff -u "$expected" <("$BIN" "$@" "$fixture"); then
        echo "golden ok: $desc"
    else
        echo "golden FAILED: $desc" >&2
        fail=1
    fi
}

check "default (squeezed)" "$here/golden/expected.txt"
check "verbose (-v)"       "$here/golden/expected-verbose.txt" -v

exit "$fail"
