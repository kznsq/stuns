# STUNS - STupid UNcompreSsor

Scans a file byte by byte and tries to unpack a compressed stream at every offset.
Whatever unpacks is saved, so raw (headerless) compressed data can be pulled out of
game archives, executables and other containers without knowing their layout.
An analysis mode then tells the real finds from the junk by looking at the output:
entropy, ASCII text and known file headers.

(c) Andrii Frolov aka kznsq, 2004-2026

## Formats

| Switch | Format | Notes |
|--------|--------|-------|
| `-deflate` | Deflate (raw, RFC 1951) | zlib and gzip streams are found 2 / 10 bytes in, past their headers |
| `-pkware` | PKWare Data Compression Library (implode) | |
| `-lzo` | LZO1X | |
| `-ucl2b`, `-ucl2d`, `-ucl2e` | UCL NRV2B, NRV2D, NRV2E | NRV2E is the default of the UPX packer |
| `-lz4` | LZ4 block (no frame header) | reported only when the block uses at least one match |
| `-lzma` | LZMA raw stream, lc=3 lp=0 pb=2 | only streams with the end marker; `.lzma` files are found 13 bytes in |
| `-aplib` | aPLib | **off by default**: its end marker is easy to hit by chance |

## Usage

```
stuns [options] <filename>
  -<format>[-]      turn a format on, or off with a trailing '-'
  -Out<prefix>      prefix for output folders (default '!Out')
  -analyze          show entropy, ASCII text and file header of every stream
  -catch            keep only streams that look real (see below)
  -entropy          show the entropy of every stream (bits per byte, 0..8)
  -entropy<max>     keep only streams with entropy up to max
  -entropy<min>:<max>, -entropy<min>:   keep only streams in this entropy range
```

Every stream found is saved as `<prefix><format>/<offset in hex>`, for example
`!OutDeflate/0007`; the prefix may include folders (`-Outresults/`).

```
$ stuns -catch test.dat
Deflate: Succeeded uncompressing 34282 bytes at: 7 (0x7), entropy 5.04, text 100% (longest 34282) <== catch: text
Deflate: Succeeded uncompressing 34283 bytes at: 5062 (0x13c6), entropy 5.04, text 100% (longest 34283) <== catch: text
32 streams skipped by the filters
```

### What counts as a catch

A stream is a catch when its output starts with a known file header (PNG, GIF, JPEG,
BMP, TIFF, RIFF/WAV/AVI, IFF, MIDI, Ogg, FLAC, MP3, tracker modules, ZIP, gzip, 7-Zip,
RAR, PDF, XML, ELF, Mach-O, Windows and DOS executables and more), or when it is at
least 64 bytes of mostly (75%+) printable ASCII with at least 3 bits per byte of entropy.
Two-byte signatures (`BM`, `MZ`) are checked against their header fields.

Measured with all formats on (including aPLib):

| File | Streams found | Kept by `-catch` |
|------|---------------|------------------|
| `test.dat` | 49 | 2 (both real streams) |
| random data, 256 KB | 1,715 | 0 |
| `/bin/ls`, 151 KB | 37,295 | 0 |
| `/bin/bash`, 1.3 MB | 170,745 | 194 |

The `/bin/bash` leftovers are "word salad": decoders running over the executable's
help texts splice pieces of them into long output with the same statistics as real
text. Entropy alone is a weak filter in both directions: junk is mostly *low* entropy
(runs of one byte or a short pattern), while real images and sound can be close to 8
(the BMP and WAV test streams are 7.84 and 7.55).

## Build

C++20 and C compilers on Windows, Linux or macOS, 32-bit or 64-bit:

```
make            # GCC / Clang / MinGW
make test
```

or with CMake (also for Visual Studio):

```
cmake -S . -B build
cmake --build build --config Release
```

## Tests

`make test` runs `tests/run.sh`: the scan results of `test.dat` and `tests/samples*.bin`
must match `tests/expected.txt`, every format must unpack the reference text exactly,
and the analysis options must report and filter as documented.

- `tests/samples.bin`: LZO, NRV2B and NRV2D made by `tests/mksamples.c`, plus the PKWare
  example stream from `PKWare/blast.c`
- `tests/samples2.bin`: the same text from reference encoders, built by `tests/mkrefs.py`:
  the official UCL 1.03 compressors (`tests/uclref.c`), the `lz4` package, Python's
  `lzma` (liblzma), and an aPLib stream checked with the independent `aplib` package
- `tests/samples3.bin`: a BMP, WAV and MIDI file compressed with Deflate, LZMA and LZ4,
  for the header detection

The LZMA and aPLib decoders were also checked against those reference decoders on 400
randomly damaged streams each. LZMA: identical output for all 70 the reference
accepts, and for the 69 cut short a prefix of the reference's partial output. aPLib:
identical output for 210; the other 3 are accepted only by a quirk of the Python
reference (a match offset of 0 copies the first output byte), which STUNS rejects.

## Versions

v1.0 (2026)
- new formats: UCL NRV2E, LZ4 block, raw LZMA, aPLib
- analysis mode: `-analyze`, `-catch`, `-entropy`
- portable: GCC, Clang and MSVC on Windows, Linux and macOS, 32-bit and 64-bit
- modern C++20: no raw owning pointers or shared static buffers; decoders take
  `std::span` input and output, files go through `std::filesystem` and streams
- the input is read into memory followed by 64 zero bytes: miniLZO 1.08 and the UCL
  decoders read up to 7 bytes past the end of their input
- puff: a dynamic block whose code length codes are all zero wrote outside a stack
  array (negative symbol used as a code length); it is now rejected as invalid
- PKWare: output larger than the 256 Mb buffer overran it, and input was handed to
  the decoder again after it had used it all; both now end the stream with an error
- miniLZO 1.08: 64-bit pointer type on 64-bit Windows
- MinGW: puff and blast use the compiler's own setjmp/longjmp
- output folders are created as needed; write errors are reported

v0.1 (2004) - first version, Win32 / MSVC 6

## Third-party code

| Directory | Library | License |
|-----------|---------|---------|
| `Deflate` | puff 1.7 from zlib (Mark Adler) | zlib |
| `PKWare`  | blast 1.1 from zlib (Mark Adler) | zlib |
| `LZO`     | miniLZO 1.08 (Markus F.X.J. Oberhumer) | GPL v2 or later |
| `UCL`     | UCL 0.92 decompressors; NRV2E from the UCL 1.03 change (Markus F.X.J. Oberhumer) | GPL v2 or later |

The LZ4, LZMA and aPLib decoders and the analysis are original code, written from the
format descriptions.
