#include <iros/format.h>
#include <iros/memory.h>
#include <iros/rtc.h>
#include <iros/status.h>
#include <iros/vga.h>

static int status_enabled = 1;

static void write_right_aligned(u8 row, const char *s) {
  u32 len = 0;
  while (s[len]) len++;
  if (len >= 80) len = 80;
  u8 col = (u8)(80 - len);
  vga_write_at(row, col, s);
}

void status_init(void) {
  status_update();
}

void status_update(void) {
  if (!status_enabled) return;
  const u8 row = 24;

  u8 old_fg, old_bg;
  vga_get_color(&old_fg, &old_bg);

  vga_set_color(0, 11);
  vga_fill_row(row, ' ');

  /* Left side: IROS + time */
  rtc_time_t t;
  rtc_read(&t);
  char ts[9];
  rtc_format_time(&t, ts);

  char left[24];
  u32 lp = 0;
  fmt_append(left, sizeof(left), &lp, " IROS ");
  fmt_append(left, sizeof(left), &lp, ts);
  fmt_append(left, sizeof(left), &lp, " ");
  vga_write_at(row, 0, left);

  /* Right side: heap stats */
  kmem_stats_t s = kmem_stats();
  char buf[96];
  u32 pos = 0;
  fmt_append(buf, sizeof(buf), &pos, "heap ");
  pos += fmt_u32_hex(buf + pos, sizeof(buf) - pos, (u32)s.used_bytes);
  fmt_append(buf, sizeof(buf), &pos, " / ");
  pos += fmt_u32_hex(buf + pos, sizeof(buf) - pos, (u32)s.free_bytes);
  fmt_append(buf, sizeof(buf), &pos, " free ");

  write_right_aligned(row, buf);

  vga_set_color(old_fg, old_bg);
}

void status_set_enabled(int enabled) {
  status_enabled = enabled ? 1 : 0;
}

int status_is_enabled(void) {
  return status_enabled;
}
