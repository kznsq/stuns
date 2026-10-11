# STUNS - STupid UNcompreSsor

Scans a file byte by byte and tries to unpack a compressed stream at every offset.
Whatever unpacks is saved, so raw (headerless) compressed data can be pulled out of
game archives, executables and other containers without knowing their layout.

Supported formats: Deflate (raw, RFC 1951), PKWare Data Compression Library (implode),
LZO1X, UCL NRV2B and NRV2D.

(c) Andrii Frolov aka kznsq, 2004

## Build

Any C and C++ compiler on Windows, Linux or macOS, 32-bit or 64-bit:

```
make            # GCC / Clang / MinGW
make test
```

or with CMake (also for Visual Studio):

```
cmake -S . -B build
cmake --build build --config Release
```

## Usage

```
stuns [options] <filename>
  -deflate[-]   Deflate format
  -pkware[-]    PKWare Compression Library format
  -lzo[-]       LZO compression library format
  -ucl2b[-]     UCL nrv2b compression library format
  -ucl2d[-]     UCL nrv2d compression library format
  -Out<prefix>  prefix for out directory (default '!Out')
```

All formats are on by default; a trailing `-` turns one off. Every stream found is
saved as `<prefix><format>/<offset in hex>`, for example `!OutDeflate/0007`.

## Versions

v0.2 (2026) - portable version of v0.1
- builds with GCC, Clang and MSVC on Windows, Linux and macOS, 32-bit and 64-bit
  (no `windows.h`, file mapping or MSVC 6 project)
- the input is read into memory followed by 64 zero bytes: miniLZO 1.08 and the UCL
  decoders read up to 7 bytes past the end of their input
- the output buffer is a memory block instead of a 256 Mb temporary file
- puff: a dynamic block whose code length codes are all zero wrote outside a stack
  array (negative symbol used as a code length); it is now rejected as invalid
- PKWare: output larger than the 256 Mb buffer overran it; such a stream now fails
- PKWare, LZO, UCL: empty output is not saved
- miniLZO 1.08: 64-bit pointer type on 64-bit Windows (it assumed long holds a pointer)
- MinGW: puff and blast use the compiler's own setjmp/longjmp (the C runtime's
  version crashes in optimized code)
- missing output folder or write errors are reported instead of crashing;
  an unreadable input file is reported
- `make test`: regression test on `test.dat` and on LZO, UCL and PKWare samples
  built by `tests/mksamples.c`; GitHub Actions builds and tests on Linux (64/32-bit),
  macOS, Visual Studio and MinGW

v0.1 (2004) - first version, Win32 / MSVC 6

## Third-party code

| Directory | Library | License |
|-----------|---------|---------|
| `Deflate` | puff 1.7 from zlib (Mark Adler) | zlib |
| `PKWare`  | blast 1.1 from zlib (Mark Adler) | zlib |
| `LZO`     | miniLZO 1.08 (Markus F.X.J. Oberhumer) | GPL v2 or later |
| `UCL`     | UCL 0.92 decompressors (Markus F.X.J. Oberhumer) | GPL v2 or later |
