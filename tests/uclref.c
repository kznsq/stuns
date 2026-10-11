/* reference streams from the official UCL 1.03 compressors: uclref <b|d|e> <in> <out> */
#include <stdio.h>
#include <stdlib.h>
#include <ucl/ucl.h>
int main(int argc, char **argv) {
  FILE *f = fopen(argv[2], "rb"); fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
  unsigned char *in = malloc(n), *out = malloc(n + n / 8 + 256); fread(in, 1, n, f); fclose(f);
  ucl_uint ol = 0; int r;
  if (ucl_init() != UCL_E_OK) return 9;
  if (argv[1][0] == 'b') r = ucl_nrv2b_99_compress(in, n, out, &ol, NULL, 10, NULL, NULL);
  else if (argv[1][0] == 'd') r = ucl_nrv2d_99_compress(in, n, out, &ol, NULL, 10, NULL, NULL);
  else r = ucl_nrv2e_99_compress(in, n, out, &ol, NULL, 10, NULL, NULL);
  if (r != UCL_E_OK) return 2;
  f = fopen(argv[3], "wb"); fwrite(out, 1, ol, f); fclose(f);
  printf("%ld -> %u\n", n, (unsigned) ol);
  return 0;
}
