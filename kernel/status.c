#include <iros/memory.h>
#include <iros/status.h>
#include <iros/vga.h>

static int status_enabled = 1;

static void write_right_aligned(u8 row, const char *s) {
  /* Compute length (ASCII only). */
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
  /* Bottom row is reserved by vga_init(). */
  const u8 row = 24;

  u8 old_fg, old_bg;
  vga_get_color(&old_fg, &old_bg);

  /* Bar: black on light-cyan (Kali-like dark + accent). */
  vga_set_color(0, 11);
  vga_fill_row(row, ' ');

  vga_write_at(row, 0, " IROS ");

  kmem_stats_t s = kmem_stats();
  char buf[64];

  /* Very small integer formatting: show used/free in hex. */
  /* Format: "heap used 0x........ free 0x........" */
  int idx = 0;
  const char *prefix = "heap used ";
  for (int i = 0; prefix[i] && idx < (int)sizeof(buf) - 1; i++) buf[idx++] = prefix[i];

  /* Append hex (8 nibbles) */
  const char *hex = "0123456789ABCDEF";
  buf[idx++] = '0';
  buf[idx++] = 'x';
  for (int shift = 28; shift >= 0 && idx < (int)sizeof(buf) - 1; shift -= 4) {
    buf[idx++] = hex[(s.used_bytes >> shift) & 0xF];
  }
  const char *mid = " free ";
  for (int i = 0; mid[i] && idx < (int)sizeof(buf) - 1; i++) buf[idx++] = mid[i];
  buf[idx++] = '0';
  buf[idx++] = 'x';
  for (int shift = 28; shift >= 0 && idx < (int)sizeof(buf) - 1; shift -= 4) {
    buf[idx++] = hex[(s.free_bytes >> shift) & 0xF];
  }
  buf[idx] = 0;

  write_right_aligned(row, buf);

  vga_set_color(old_fg, old_bg);
}

void status_set_enabled(int enabled) {
  status_enabled = enabled ? 1 : 0;
}

int status_is_enabled(void) {
  return status_enabled;
}
