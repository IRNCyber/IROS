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

