#include <iros/vga.h>
#include <iros/ports.h>

enum { VGA_WIDTH = 80, VGA_HEIGHT = 25 };
static volatile u16 *const VGA_MEMORY = (u16 *)0xB8000;

static vga_cursor_t cursor = {0, 0};
static u8 text_attr = 0x0F; /* white on black */
static u8 scroll_height = VGA_HEIGHT; /* may reserve bottom row */

static inline u16 vga_entry(char c, u8 attr) {
  return (u16)c | ((u16)attr << 8);
}

static void vga_hw_set_cursor(u16 pos) {
  outb(0x3D4, 0x0F);
  outb(0x3D5, (u8)(pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}

static u16 vga_hw_pos(vga_cursor_t c) {
  return (u16)c.row * (u16)VGA_WIDTH + (u16)c.col;
}

static void scroll_if_needed(void) {
  if (cursor.row < scroll_height) return;
  for (u32 r = 1; r < (u32)scroll_height; r++) {
    for (u32 c = 0; c < VGA_WIDTH; c++) {
      VGA_MEMORY[(r - 1) * VGA_WIDTH + c] = VGA_MEMORY[r * VGA_WIDTH + c];
    }
  }
  for (u32 c = 0; c < VGA_WIDTH; c++) {
    VGA_MEMORY[((u32)scroll_height - 1u) * VGA_WIDTH + c] = vga_entry(' ', text_attr);
  }
  cursor.row = (u8)(scroll_height - 1);
}

void vga_init(void) {
  vga_set_color(15, 0);
  /* Reserve last row for a status bar (Kali-like UI). */
  scroll_height = VGA_HEIGHT - 1;
  vga_clear();
}

void vga_set_color(u8 fg, u8 bg) {
  text_attr = (u8)((bg << 4) | (fg & 0x0F));
}

void vga_get_color(u8 *fg, u8 *bg) {
  if (fg) *fg = (u8)(text_attr & 0x0F);
  if (bg) *bg = (u8)((text_attr >> 4) & 0x0F);
}

void vga_set_cursor(vga_cursor_t c) {
  if (c.row >= scroll_height) c.row = (u8)(scroll_height - 1);
  if (c.col >= VGA_WIDTH) c.col = VGA_WIDTH - 1;
  cursor = c;
  vga_hw_set_cursor(vga_hw_pos(cursor));
}

vga_cursor_t vga_get_cursor(void) {
  return cursor;
}

void vga_clear(void) {
  for (u32 i = 0; i < (u32)VGA_WIDTH * (u32)VGA_HEIGHT; i++) {
    VGA_MEMORY[i] = vga_entry(' ', text_attr);
  }
  cursor.row = 0;
  cursor.col = 0;
  vga_hw_set_cursor(0);
}

void vga_putc(char ch) {
  if (ch == '\n') {
    cursor.col = 0;
    cursor.row++;
    scroll_if_needed();
    vga_hw_set_cursor(vga_hw_pos(cursor));
    return;
  }
  if (ch == '\r') {
    cursor.col = 0;
    vga_hw_set_cursor(vga_hw_pos(cursor));
    return;
  }
  if (ch == '\b') {
    if (cursor.col > 0) {
      cursor.col--;
    } else if (cursor.row > 0) {
      cursor.row--;
      cursor.col = VGA_WIDTH - 1;
    }
    VGA_MEMORY[cursor.row * VGA_WIDTH + cursor.col] = vga_entry(' ', text_attr);
    vga_hw_set_cursor(vga_hw_pos(cursor));
    return;
  }

  VGA_MEMORY[cursor.row * VGA_WIDTH + cursor.col] = vga_entry(ch, text_attr);
  cursor.col++;
  if (cursor.col >= VGA_WIDTH) {
    cursor.col = 0;
    cursor.row++;
    scroll_if_needed();
  }
  vga_hw_set_cursor(vga_hw_pos(cursor));
}

void vga_write(const char *s) {
  for (u32 i = 0; s[i]; i++) {
    vga_putc(s[i]);
  }
}

static int is_digit(char c) { return c >= '0' && c <= '9'; }

static void ansi_apply_sgr(int code) {
  u8 fg, bg;
  vga_get_color(&fg, &bg);

  if (code == 0) {
    fg = 15;
    bg = 0;
  } else if (code == 1) {
    /* Bold -> bright fg. */
    fg = (u8)(fg | 0x08);
  } else if (code >= 30 && code <= 37) {
    fg = (u8)(code - 30);
  } else if (code >= 90 && code <= 97) {
    fg = (u8)((code - 90) | 0x08);
  } else if (code >= 40 && code <= 47) {
    bg = (u8)(code - 40);
  } else if (code >= 100 && code <= 107) {
    bg = (u8)((code - 100) | 0x08);
  }

  vga_set_color(fg, bg);
}

void vga_write_ansi(const char *s) {
  /* Minimal ANSI parser: supports CSI ... m (SGR colors). */
  for (u32 i = 0; s[i]; ) {
    unsigned char ch = (unsigned char)s[i];
    if (ch != 0x1B) {
      vga_putc((char)ch);
      i++;
      continue;
    }

    /* ESC [ ... m */
    if (s[i + 1] != '[') {
      i++;
      continue;
    }

    i += 2;
    int any = 0;
    int code = 0;

    for (;;) {
      char c = s[i];
      if (!c) return;
      if (is_digit(c)) {
        any = 1;
        code = code * 10 + (c - '0');
        i++;
        continue;
      }
      if (c == ';') {
        ansi_apply_sgr(any ? code : 0);
        any = 0;
        code = 0;
        i++;
        continue;
      }
      if (c == 'm') {
        ansi_apply_sgr(any ? code : 0);
        i++;
        break;
      }

      /* Unsupported CSI command: skip it. */
      i++;
      break;
    }
  }
}

void vga_write_at(u8 row, u8 col, const char *s) {
  if (row >= VGA_HEIGHT) return;
  if (col >= VGA_WIDTH) return;
  for (u32 i = 0; s[i]; i++) {
    u32 x = (u32)col + i;
    if (x >= VGA_WIDTH) break;
    VGA_MEMORY[(u32)row * VGA_WIDTH + x] = vga_entry(s[i], text_attr);
  }
}

void vga_fill_row(u8 row, char ch) {
  if (row >= VGA_HEIGHT) return;
  for (u32 col = 0; col < VGA_WIDTH; col++) {
    VGA_MEMORY[(u32)row * VGA_WIDTH + col] = vga_entry(ch, text_attr);
  }
}

void vga_write_hex(u32 value) {
  static const char *hex = "0123456789ABCDEF";
  vga_write("0x");
  for (int shift = 28; shift >= 0; shift -= 4) {
    vga_putc(hex[(value >> shift) & 0xF]);
  }
}
