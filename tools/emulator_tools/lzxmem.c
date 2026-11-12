/* Minimal in-memory LZX decompressor around libmspack's lzxd (as Xenia does for XEX2 images).
   usage: lzxmem <in> <out> <window_size> <out_len> */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mspack.h"
#include "lzx.h"

struct memfile { unsigned char *buf; size_t len, pos; };
static struct mspack_file *m_open(struct mspack_system *s, const char *f, int m) { return NULL; }
static void m_close(struct mspack_file *f) {}
static int m_read(struct mspack_file *f, void *b, int n) {
  struct memfile *m = (struct memfile *)f; size_t left = m->len - m->pos;
  if ((size_t)n > left) n = (int)left; memcpy(b, m->buf + m->pos, n); m->pos += n; return n; }
static int m_write(struct mspack_file *f, void *b, int n) {
  struct memfile *m = (struct memfile *)f; size_t left = m->len - m->pos;
  if ((size_t)n > left) n = (int)left; memcpy(m->buf + m->pos, b, n); m->pos += n; return n; }
static int m_seek(struct mspack_file *f, off_t o, int mode) { return -1; }
static off_t m_tell(struct mspack_file *f) { return ((struct memfile *)f)->pos; }
static void m_msg(struct mspack_file *f, const char *fmt, ...) {}
static void *m_alloc(struct mspack_system *s, size_t n) { return malloc(n); }
static void m_free(void *p) { free(p); }
static void m_copy(void *a, void *b, size_t n) { memmove(b, a, n); }
static struct mspack_system sys = { m_open, m_close, m_read, m_write, m_seek, m_tell, m_msg, m_alloc, m_free, m_copy, NULL };

int main(int argc, char **argv) {
  if (argc < 5) return 2;
  FILE *fi = fopen(argv[1], "rb"); fseek(fi, 0, SEEK_END); long il = ftell(fi); fseek(fi, 0, SEEK_SET);
  unsigned char *in = malloc(il); fread(in, 1, il, fi); fclose(fi);
  unsigned long ws = strtoul(argv[3], 0, 0), ol = strtoul(argv[4], 0, 0);
  int wb = 0; for (unsigned long t = ws; t > 1; t >>= 1) wb++;
  unsigned char *out = calloc(1, ol);
  struct memfile src = { in, (size_t)il, 0 }, dst = { out, ol, 0 };
  struct lzxd_stream *z = lzxd_init(&sys, (struct mspack_file *)&src, (struct mspack_file *)&dst, wb, 0, 0x8000, (off_t)ol, 0);
  if (!z) { fprintf(stderr, "init failed wb=%d\n", wb); return 1; }
  int r = lzxd_decompress(z, (off_t)ol);
  lzxd_free(z);
  FILE *fo = fopen(argv[2], "wb"); fwrite(out, 1, ol, fo); fclose(fo);
  fprintf(stderr, "lzx result %d, wrote %lu bytes (window bits %d)\n", r, ol, wb);
  return r;
}
