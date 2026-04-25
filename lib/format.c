#include <iros/format.h>

u32 fmt_u32_dec(char *buf, u32 bufsize, u32 value) {
  if (bufsize == 0) return 0;
  if (value == 0) {
    buf[0] = '0';
    if (bufsize > 1) buf[1] = 0;
    return 1;
  }

  char tmp[11];
  u32 n = 0;
  while (value && n < sizeof(tmp)) {
    tmp[n++] = (char)('0' + (value % 10u));
    value /= 10u;
  }

  u32 written = 0;
  while (n > 0 && written + 1 < bufsize) {
    buf[written++] = tmp[--n];
  }
  if (written < bufsize) buf[written] = 0;
  return written;
}

u32 fmt_u32_hex(char *buf, u32 bufsize, u32 value) {
  static const char hex[] = "0123456789ABCDEF";
  if (bufsize < 3) return 0;
  u32 pos = 0;
  buf[pos++] = '0';
  buf[pos++] = 'x';
  for (int shift = 28; shift >= 0 && pos + 1 < bufsize; shift -= 4) {
    buf[pos++] = hex[(value >> shift) & 0xF];
  }
  if (pos < bufsize) buf[pos] = 0;
  return pos;
}

void fmt_append(char *buf, u32 bufsize, u32 *pos, const char *s) {
  for (u32 i = 0; s && s[i] && *pos + 1 < bufsize; i++) {
    buf[(*pos)++] = s[i];
  }
  if (*pos < bufsize) buf[*pos] = 0;
}
