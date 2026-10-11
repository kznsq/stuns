STUNS scans a file byte by byte and unpacks every compressed stream it finds, without
needing headers; the analysis mode tells real finds from junk by their content.

**New in 1.0**
- new formats: UCL NRV2E, LZ4 block, raw LZMA, aPLib (`-aplib`, off by default)
- analysis: `-analyze` shows entropy, ASCII text and file headers of every stream;
  `-catch` keeps only streams that look real; `-entropy<min>:<max>` filters by entropy
- portable: Windows, Linux and macOS, 32-bit and 64-bit; modern C++20
- fixes in the 2004 libraries: a puff stack overwrite on invalid input, reads past the
  input end in miniLZO 1.08 and UCL, PKWare output overrun, 64-bit Windows miniLZO

Binaries: `stuns-windows-x64.exe`, `stuns-linux-x64` (static), `stuns-macos-universal`
(macOS 13.3 or newer). See README.md for usage and the measured catch rates.
