/*
  Builds tests/samples.bin: the plain text compressed with LZO, UCL nrv2b and UCL nrv2d,
  plus the PKWare example stream from PKWare/blast.c, each placed between filler bytes
  so stuns has to find it by scanning.

  usage: mksamples <plain.txt> <samples.bin>

  LZO uses minilzo's own compressor. The nrv2b / nrv2d encoders below are simple
  greedy encoders written from the decoders in UCL/n2b_d.c and UCL/n2d_d.c:
  they only need to produce valid streams, not small ones.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../LZO/minilzo.h"

/* ---- bit writer matching UCL getbit_8 (8 bits per byte, MSB first, bytes interleaved) ---- */

typedef struct {
  uint8_t *dat;
  size_t len;
  size_t bitpos;
  int bitcnt;
} tWriter;

static void PutByte(tWriter *w, uint8_t b) {
  w->dat[w->len++] = b;
}

static void PutBit(tWriter *w, int bit) {
  if (!w->bitcnt) {
    w->bitpos = w->len;
    PutByte(w, 0);
    w->bitcnt = 8;
  }
  w->bitcnt--;
  w->dat[w->bitpos] |= (uint8_t) ((bit & 1) << w->bitcnt);
}

/* nrv2b gamma: do { x = x*2 + bit; } while (!bit); */
static void PutGamma2b(tWriter *w, uint32_t x) {
int n;
  for (n = 31; !((x >> n) & 1); n--);
  for (n--; n >= 0; n--) {
    PutBit(w, (x >> n) & 1);
    PutBit(w, !n);
  }
}

/* nrv2d offset gamma: for (;;) { x = x*2 + bit; if (bit) break; x = (x-1)*2 + bit; } */
static void PutGamma2d(tWriter *w, uint32_t x) {
uint32_t bits[64];
int n, i;
  n = 0;
  while (x > 3) {
    /* x = m*2 + b, m = (a-1)*2 + c */
    bits[n++] = x & 1;
    bits[n++] = (x >> 1) & 1;
    x = (((x >> 1) - ((x >> 1) & 1)) >> 1) + 1;
  }
  PutBit(w, x & 1);
  for (i = n - 1; i >= 0; i -= 2) {
    PutBit(w, 0);
    PutBit(w, bits[i]);
    PutBit(w, bits[i - 1]);
  }
  PutBit(w, 1);
}

/* match length code: 1..3 in two bits, otherwise 00 + gamma(code - 2) */
static void PutLength(tWriter *w, uint32_t code, int skipfirst) {
  if (code < 4) {
    if (!skipfirst) {
      PutBit(w, code >> 1);
    }
    PutBit(w, code & 1);
  } else {
    if (!skipfirst) {
      PutBit(w, 0);
    }
    PutBit(w, 0);
    PutGamma2b(w, code - 2);
  }
}

static size_t FindMatch(const uint8_t *s, size_t len, size_t pos, size_t *off) {
size_t i, j, best;
  best = 0;
  *off = 0;
  for (i = (pos > 0x8000) ? pos - 0x8000 : 0; i < pos; i++) {
    for (j = 0; (pos + j < len) && (s[i + j] == s[pos + j]); j++);
    if (j >= best) {
      best = j;
      *off = pos - i;
    }
  }
  return(best);
}

/* variant: 'b' for nrv2b, 'd' for nrv2d */
static size_t CompressNRV(const uint8_t *s, size_t len, uint8_t *out, int variant) {
tWriter w;
size_t pos, mlen, off, last, far, code, v;
  memset(&w, 0, sizeof(w));
  w.dat = out;
  last = 1;
  pos = 0;
  while (pos < len) {
    mlen = FindMatch(s, len, pos, &off);
    far = (variant == 'b') ? (off > 0xD00) : (off > 0x500);
    if (mlen < 2 + far) {
      /* literal */
      PutBit(&w, 1);
      PutByte(&w, s[pos++]);
      continue;
    }
    PutBit(&w, 0);
    code = mlen - 1 - far;
    if (off == last) {
      if (variant == 'b') {
        PutGamma2b(&w, 2);
      } else {
        PutGamma2d(&w, 2);
      }
      PutLength(&w, code, 0);
    } else if (variant == 'b') {
      PutGamma2b(&w, (uint32_t) ((off - 1) >> 8) + 3);
      PutByte(&w, (uint8_t) (off - 1));
      PutLength(&w, code, 0);
    } else {
      /* the first length bit is stored inverted in the low bit of the offset */
      v = ((off - 1) << 1) | (((code < 4) ? (code >> 1) : 0) ^ 1);
      PutGamma2d(&w, (uint32_t) (v >> 8) + 3);
      PutByte(&w, (uint8_t) v);
      PutLength(&w, code, 1);
    }
    last = off;
    pos += mlen;
  }
  /* end marker: offset 0xFFFFFFFF */
  PutBit(&w, 0);
  if (variant == 'b') {
    PutGamma2b(&w, 0x1000002UL);
  } else {
    PutGamma2d(&w, 0x1000002UL);
  }
  PutByte(&w, 0xFF);
  return(w.len);
}

static void PutFiller(FILE *fl, size_t n) {
  /* 0x00 bytes never start a valid stream of these formats long enough to pass -skip */
  while (n--) {
    fputc(0, fl);
  }
}

int main(int argc, char *argv[]) {
FILE *fl;
uint8_t *s, *u, wrk[LZO1X_1_MEM_COMPRESS];
size_t len, n;
lzo_uint ul;
  if (argc != 3) {
    printf("usage: mksamples <plain.txt> <samples.bin>\n");
    return(1);
  }
  fl = fopen(argv[1], "rb");
  if (!fl) {
    return(2);
  }
  fseek(fl, 0, SEEK_END);
  len = (size_t) ftell(fl);
  fseek(fl, 0, SEEK_SET);
  s = (uint8_t *) malloc(len);
  u = (uint8_t *) malloc(len * 2 + 1024);
  if (!s || !u || (fread(s, 1, len, fl) != len)) {
    return(3);
  }
  fclose(fl);
  fl = fopen(argv[2], "wb");
  if (!fl || (lzo_init() != LZO_E_OK)) {
    return(4);
  }
  PutFiller(fl, 0x100);
  ul = 0;
  lzo1x_1_compress(s, len, u, &ul, wrk);
  fwrite(u, 1, ul, fl);
  PutFiller(fl, 0x100);
  n = CompressNRV(s, len, u, 'b');
  fwrite(u, 1, n, fl);
  PutFiller(fl, 0x100);
  n = CompressNRV(s, len, u, 'd');
  fwrite(u, 1, n, fl);
  PutFiller(fl, 0x100);
  /* PKWare example from blast.c, unpacks to "AIAIAIAIAIAIA" */
  fwrite("\x00\x04\x82\x24\x25\x8f\x80\x7f", 1, 8, fl);
  PutFiller(fl, 0x100);
  fclose(fl);
  free(u);
  free(s);
  return(0);
}
