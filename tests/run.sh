#!/bin/sh
# Regression test: tests/run.sh <path to stuns binary>
# 1. scan results for test.dat and tests/samples*.bin must match tests/expected.txt
# 2. every format must unpack the reference text from test.dat
# 3. -analyze / -catch / -entropy report and filter as documented
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

for f in test.dat tests/samples.bin tests/samples2.bin tests/samples3.bin; do
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

# reference encoders (tests/mkrefs.py): official UCL 1.03, lz4, liblzma, and aPLib
for f in UCL2B/0100 UCL2D/1593 UCL2E/2a53 LZ4/3eef LZ4/5ef1 LZMA/7757 LZMA/85a9; do
  cmp -s "all_$f" all_Deflate/0007 || fail "samples2.bin: $f does not match the reference text"
done
[ ! -d all_aPLib ] || fail "aPLib is on by default"
scan -deflate- -pkware- -lzo- -ucl2b- -ucl2d- -ucl2e- -lz4- -lzma- -aplib -Outap_ "$ROOT/tests/samples2.bin" > /dev/null
cmp -s ap_aPLib/93ee all_Deflate/0007 || fail "samples2.bin: aPLib/93ee does not match the reference text"

# PKWare example stream from blast.c
[ "$(cat all_PKWare/484a)" = "AIAIAIAIAIAIA" ] || fail "PKWare example stream"

# switching formats off, and the -Out prefix
[ -z "$(scan -lzo- -ucl2b- -ucl2d- -ucl2e- -lz4- -lzma- -deflate- -Outoff_ "$ROOT/tests/samples.bin" | grep -v '^PKWare:')" ] || fail "formats switched off"
scan -deflate- -pkware- -ucl2b- -ucl2d- -ucl2e- -lz4- -lzma- -Outsub/pre_ "$ROOT/tests/samples.bin" > /dev/null
[ -f sub/pre_LZO/0100 ] || fail "-Out prefix with a folder"

# analysis: the reference text, the real streams only, and file headers
scan -analyze -deflate -Outan_ "$ROOT/test.dat" | grep -q "at: 7 (0x7), entropy 5.04, text 100% (longest 34282) <== catch: text$" || fail "-analyze"
[ "$(scan -catch -Outca_ "$ROOT/test.dat" | wc -l | tr -d ' ')" = 2 ] || fail "-catch on test.dat keeps the 2 real streams only"
[ "$(scan -catch -Outhd_ "$ROOT/tests/samples3.bin" | sed 's/.*catch: //' | tr '\n' ',')" = "BMP image,RIFF WAVE file,MIDI music," ] || fail "-catch file headers"
[ "$(scan -entropy7.6 -Outen_ "$ROOT/tests/samples3.bin" | grep -c 'BMP image\|entropy 7.8')" = 0 ] || fail "-entropy<max> keeps a stream above max"
[ "$(scan -entropy7.8: -Outem_ "$ROOT/tests/samples3.bin" | wc -l | tr -d ' ')" = 1 ] || fail "-entropy<min>: keeps only the BMP"
"$STUNS" -entropy7x "$ROOT/test.dat" > /dev/null
[ $? != 0 ] || fail "-entropy with a bad number is rejected"

if [ $FAILED = 0 ]; then
  echo "All tests passed."
fi
exit $FAILED
