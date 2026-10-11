"""
Builds tests/samples2.bin: one text compressed by reference encoders, each stream
placed between 0x100 filler bytes so stuns has to find it by scanning.

usage: python3 mkrefs.py <plain.txt> <nrv2b.bin> <nrv2d.bin> <nrv2e.bin> <samples2.bin> <samples3.bin>

  plain.txt        the first Deflate stream of test.dat, unpacked
  nrv2?.bin        plain.txt compressed by the official UCL 1.03 compressors
                   (tests/uclref.c built against the UCL 1.03 source)
  LZ4              python "lz4" package, raw blocks (default and high compression)
  LZMA             python "lzma" (liblzma): raw LZMA1 lc=3 lp=0 pb=2, and .lzma
                   ("alone") format, whose stream starts 13 bytes in
  aPLib            the simple encoder below; checked with the independent "aplib"
                   package decoder when it is installed

Also builds tests/samples3.bin for the -analyze / -catch header checks: a generated
BMP image (raw Deflate), WAV sound (raw LZMA) and MIDI file (LZ4 block).

Prints the offset of every stream for tests/run.sh.
"""
import lzma
import math
import struct
import sys
import zlib

import lz4.block


class BitWriter:
    """aPLib tag bits: 8 per byte, most significant first, bytes interleaved with data"""

    def __init__(self):
        self.out = bytearray()
        self.tagpos = 0
        self.bits = 0

    def byte(self, b):
        self.out.append(b)

    def bit(self, b):
        if not self.bits:
            self.tagpos = len(self.out)
            self.out.append(0)
            self.bits = 8
        self.bits -= 1
        self.out[self.tagpos] |= (b & 1) << self.bits

    def gamma(self, v):
        assert v >= 2
        n = v.bit_length() - 1
        for i in range(n - 1, -1, -1):
            self.bit((v >> i) & 1)
            self.bit(1 if i else 0)


def aplib_compress(data):
    """greedy aPLib encoder using every code type; valid, not small"""
    w = BitWriter()
    w.byte(data[0])
    chains = {}
    lwm, r0 = 0, 0
    pos = 1

    def add(p):
        if p + 3 <= len(data):
            chains.setdefault(bytes(data[p:p + 3]), []).append(p)

    add(0)
    while pos < len(data):
        best_len, best_off = 0, 0
        for cand in reversed(chains.get(bytes(data[pos:pos + 3]), [])[-64:]):
            n = 0
            while pos + n < len(data) and data[cand + n] == data[pos + n]:
                n += 1
            if n > best_len:
                best_len, best_off = n, pos - cand
        rep_len = 0
        if not lwm and r0 and r0 <= pos:
            while pos + rep_len < len(data) and data[pos + rep_len - r0] == data[pos + rep_len]:
                rep_len += 1
        adj = (best_off >= 32000) + (best_off >= 1280) + (2 if best_off < 128 else 0)
        if rep_len >= 2 and rep_len >= best_len:
            # 10 + gamma(2): repeat the last offset
            w.bit(1); w.bit(0); w.gamma(2); w.gamma(rep_len)
            n, lwm = rep_len, 1
        elif best_len >= 2 and best_len <= 3 and best_off < 128:
            # 110: short match
            w.bit(1); w.bit(1); w.bit(0); w.byte((best_off << 1) | (best_len - 2))
            n, lwm, r0 = best_len, 1, best_off
        elif best_len >= 2 + adj and best_len - adj >= 2:
            # 10: gamma-coded offset
            w.bit(1); w.bit(0)
            w.gamma((best_off >> 8) + (2 if lwm else 3))
            w.byte(best_off & 0xFF)
            w.gamma(best_len - adj)
            n, lwm, r0 = best_len, 1, best_off
        else:
            b = data[pos]
            near = [o for o in range(1, min(15, pos) + 1) if data[pos - o] == b]
            if b == 0 or near:
                # 111: single byte, 4-bit offset (0 writes a zero byte)
                o = 0 if b == 0 else near[0]
                w.bit(1); w.bit(1); w.bit(1)
                for i in range(3, -1, -1):
                    w.bit((o >> i) & 1)
            else:
                # 0: literal
                w.bit(0); w.byte(b)
            n, lwm = 1, 0
        for p in range(pos, pos + n):
            add(p)
        pos += n
    # end marker: 110 with offset byte 0
    w.bit(1); w.bit(1); w.bit(0); w.byte(0)
    return bytes(w.out)


def make_bmp():
    w = h = 64
    pixels = bytes((x * 4 + y) & 0xFF for y in range(h) for x in range(w))
    palette = b''.join(bytes((i, i, 255 - i, 0)) for i in range(256))
    offset = 14 + 40 + len(palette)
    info = struct.pack('<IiiHHIIiiII', 40, w, h, 1, 8, 0, len(pixels), 2835, 2835, 256, 0)
    return b'BM' + struct.pack('<IHHI', offset + len(pixels), 0, 0, offset) + info + palette + pixels


def make_wav():
    rate, n = 11025, 11025
    samples = b''.join(struct.pack('<h', int(12000 * math.sin(2 * math.pi * 440 * i / rate))) for i in range(n))
    fmt = struct.pack('<HHIIHH', 1, 1, rate, rate * 2, 2, 16)
    body = b'WAVE' + b'fmt ' + struct.pack('<I', len(fmt)) + fmt + b'data' + struct.pack('<I', len(samples)) + samples
    return b'RIFF' + struct.pack('<I', len(body)) + body


def make_midi():
    track = b''.join(bytes((0, 0x90, 60 + i % 12, 100, 0x60, 0x80, 60 + i % 12, 0)) for i in range(200)) + b'\x00\xff\x2f\x00'
    return b'MThd' + struct.pack('>IHHH', 6, 0, 1, 96) + b'MTrk' + struct.pack('>I', len(track)) + track


def write_headers_sample(path):
    deflate = zlib.compressobj(9, zlib.DEFLATED, -15)
    streams = [('BMP / Deflate', deflate.compress(make_bmp()) + deflate.flush()),
               ('WAV / LZMA', lzma.compress(make_wav(), format=lzma.FORMAT_RAW,
                                            filters=[{'id': lzma.FILTER_LZMA1, 'lc': 3, 'lp': 0, 'pb': 2}])),
               ('MIDI / LZ4', lz4.block.compress(make_midi(), store_size=False))]
    out = bytearray(0x100)
    for name, data in streams:
        print('%-13s at 0x%04x, %d bytes' % (name, len(out), len(data)))
        out += data + bytes(0x100)
    open(path, 'wb').write(out)


def main():
    plain = open(sys.argv[1], 'rb').read()
    streams = [('UCL2B', open(sys.argv[2], 'rb').read()),
               ('UCL2D', open(sys.argv[3], 'rb').read()),
               ('UCL2E', open(sys.argv[4], 'rb').read()),
               ('LZ4', lz4.block.compress(plain, store_size=False)),
               ('LZ4 high', lz4.block.compress(plain, mode='high_compression', store_size=False)),
               ('LZMA raw', lzma.compress(plain, format=lzma.FORMAT_RAW,
                                          filters=[{'id': lzma.FILTER_LZMA1, 'lc': 3, 'lp': 0, 'pb': 2,
                                                    'dict_size': 1 << 16}])),
               ('LZMA alone', lzma.compress(plain, format=lzma.FORMAT_ALONE)),
               ('aPLib', aplib_compress(plain))]
    try:
        import aplib
        assert aplib.decompress(streams[-1][1]) == plain, 'aPLib stream rejected by the reference decoder'
        print('aPLib stream checked with the reference decoder')
    except ImportError:
        print('aplib package not installed: aPLib stream not checked')
    out = bytearray(0x100)
    for name, data in streams:
        print('%-10s at 0x%04x, %d bytes' % (name, len(out), len(data)))
        out += data + bytes(0x100)
    open(sys.argv[5], 'wb').write(out)
    write_headers_sample(sys.argv[6])


if __name__ == '__main__':
    main()
