#include <iros/log.h>
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

void log_info(const char *msg) {
  log_prefix("[INFO]", 10);
  vga_write(msg);
  vga_write("\n");
}

void log_error(const char *msg) {
  log_prefix("[ERR ]", 12);
  vga_write(msg);
  vga_write("\n");
}

