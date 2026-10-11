#!/bin/sh
# Regression test: tests/run.sh <path to stuns binary>
# 1. scan results for test.dat and tests/samples.bin must match tests/expected.txt
# 2. every format must unpack the reference text from test.dat
STUNS=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
# output names stay relative: Windows builds do not understand /tmp inside "-Out/tmp/..."
cd "$TMP" || exit 1
FAILED=0

fail() {
  echo "FAIL: $*"
  FAILED=1
}

# result lines only, CR removed for Windows builds
scan() {
  "$STUNS" "$@" | tr -d '\r' | grep "Succeeded"
}

for f in test.dat tests/samples.bin; do
  echo "== $f"
  scan -Outall_ "$ROOT/$f"
done > listing.txt
diff "$ROOT/tests/expected.txt" listing.txt > listing.diff || {
  fail "scan results differ from tests/expected.txt"
  cat listing.diff
}

# reference text: the first Deflate stream in test.dat
[ "$(cksum < all_Deflate/0007 | tr -s ' \t' ' ')" = "3685922246 34282" ] || fail "Deflate reference text checksum"

# LZO, UCL nrv2b and nrv2d copies of the same text
for f in LZO/0100 UCL2B/1dfd UCL2D/330e; do
  cmp -s "all_$f" all_Deflate/0007 || fail "$f does not match the reference text"
done

# PKWare example stream from blast.c
[ "$(cat all_PKWare/484a)" = "AIAIAIAIAIAIA" ] || fail "PKWare example stream"

# switching formats off, and the -Out prefix
[ -z "$(scan -lzo- -ucl2b- -ucl2d- -deflate- -Outoff_ "$ROOT/tests/samples.bin" | grep -v '^PKWare:')" ] || fail "formats switched off"
scan -deflate- -pkware- -ucl2b- -ucl2d- -Outpre_ "$ROOT/tests/samples.bin" > /dev/null
[ -f pre_LZO/0100 ] || fail "-Out prefix"

if [ $FAILED = 0 ]; then
  echo "All tests passed."
fi
exit $FAILED
