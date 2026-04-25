#include <iros/log.h>
#include <iros/serial.h>
#include <iros/vga.h>

static void log_prefix(const char *tag, unsigned fg) {
  vga_set_color((u8)fg, 0);
  vga_write(tag);
  vga_set_color(15, 0);
  vga_write(" ");
}

void log_init(void) {
  /* nothing for now */
}

enum { DMESG_SIZE = 4096 };
static char dmesg_buf[DMESG_SIZE];
static u32 dmesg_head = 0;
static u32 dmesg_len = 0;
static int dmesg_serial = 0;

static void dmesg_putc(char c) {
  dmesg_buf[dmesg_head] = c;
  dmesg_head = (dmesg_head + 1u) % DMESG_SIZE;
  if (dmesg_len < DMESG_SIZE) dmesg_len++;
}

static void dmesg_write(const char *s) {
  for (u32 i = 0; s && s[i]; i++) dmesg_putc(s[i]);
}

static void log_emit(const char *tag, unsigned fg, const char *msg) {
  log_prefix(tag, fg);
  vga_write(msg);
  vga_write("\n");

  dmesg_write(tag);
  dmesg_putc(' ');
  dmesg_write(msg);
  dmesg_putc('\n');

  if (dmesg_serial) {
    serial_write(tag);
    serial_write(" ");
    serial_write(msg);
    serial_write("\n");
  }
}

void log_info(const char *msg) {
  log_emit("[INFO]", 10, msg);
}

void log_error(const char *msg) {
  log_emit("[ERR ]", 12, msg);
}

void log_set_serial_enabled(int enabled) {
  dmesg_serial = enabled ? 1 : 0;
}

int log_serial_enabled(void) {
  return dmesg_serial;
}

void log_dmesg_dump(void) {
  /* Print oldest -> newest. */
  u32 start = (dmesg_len == DMESG_SIZE) ? dmesg_head : 0;
  for (u32 i = 0; i < dmesg_len; i++) {
    u32 idx = (start + i) % DMESG_SIZE;
    vga_putc(dmesg_buf[idx]);
  }
  if (dmesg_len && dmesg_buf[(start + dmesg_len - 1u) % DMESG_SIZE] != '\n') vga_write("\n");
}
