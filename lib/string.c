#include <iros/string.h>

void *memset(void *dst, int value, usize count) {
  u8 *p = (u8 *)dst;
  for (usize i = 0; i < count; i++) {
    p[i] = (u8)value;
  }
  return dst;
}

void *memcpy(void *dst, const void *src, usize count) {
  u8 *d = (u8 *)dst;
  const u8 *s = (const u8 *)src;
  for (usize i = 0; i < count; i++) {
    d[i] = s[i];
  }
  return dst;
}

usize strlen(const char *s) {
  usize n = 0;
  while (s[n]) n++;
  return n;
}

int strcmp(const char *a, const char *b) {
  usize i = 0;
  while (a[i] && b[i]) {
    if (a[i] != b[i]) return (int)((unsigned char)a[i] - (unsigned char)b[i]);
    i++;
  }
  return (int)((unsigned char)a[i] - (unsigned char)b[i]);
}

int strncmp(const char *a, const char *b, usize n) {
  for (usize i = 0; i < n; i++) {
    unsigned char ac = (unsigned char)a[i];
    unsigned char bc = (unsigned char)b[i];
    if (ac != bc) return (int)(ac - bc);
    if (ac == 0) return 0;
  }
  return 0;
}
