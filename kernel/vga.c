#include <iros/vga.h>

enum { VGA_WIDTH = 80, VGA_HEIGHT = 25 };
static volatile u16 *const VGA_MEMORY = (u16 *)0xB8000;

static u8 cursor_row = 0;
static u8 cursor_col = 0;
static u8 text_attr = 0x0F; /* white on black */

static inline u16 vga_entry(char c, u8 attr) {
  return (u16)c | ((u16)attr << 8);
}

static void scroll_if_needed(void) {
  if (cursor_row < VGA_HEIGHT) return;
  for (u32 r = 1; r < VGA_HEIGHT; r++) {
    for (u32 c = 0; c < VGA_WIDTH; c++) {
      VGA_MEMORY[(r - 1) * VGA_WIDTH + c] = VGA_MEMORY[r * VGA_WIDTH + c];
    }
  }
  for (u32 c = 0; c < VGA_WIDTH; c++) {
    VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + c] = vga_entry(' ', text_attr);
  }
  cursor_row = VGA_HEIGHT - 1;
}

static void putc(char ch) {
  if (ch == '\n') {
    cursor_col = 0;
    cursor_row++;
    scroll_if_needed();
    return;
  }
  if (ch == '\r') {
    cursor_col = 0;
    return;
  }
  VGA_MEMORY[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(ch, text_attr);
  cursor_col++;
  if (cursor_col >= VGA_WIDTH) {
    cursor_col = 0;
    cursor_row++;
    scroll_if_needed();
  }
}

void vga_clear(void) {
  cursor_row = 0;
  cursor_col = 0;
  for (u32 i = 0; i < (u32)VGA_WIDTH * (u32)VGA_HEIGHT; i++) {
    VGA_MEMORY[i] = vga_entry(' ', text_attr);
  }
}

void vga_write(const char *s) {
  for (u32 i = 0; s[i]; i++) {
    putc(s[i]);
  }
}

void vga_write_hex(u32 value) {
  static const char *hex = "0123456789ABCDEF";
  vga_write("0x");
  for (int shift = 28; shift >= 0; shift -= 4) {
    putc(hex[(value >> shift) & 0xF]);
  }
}

