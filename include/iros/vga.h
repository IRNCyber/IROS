#pragma once

#include <iros/types.h>

typedef struct {
  u8 row;
  u8 col;
} vga_cursor_t;

void vga_init(void);
void vga_clear(void);
void vga_set_color(u8 fg, u8 bg);
void vga_get_color(u8 *fg, u8 *bg);
void vga_set_cursor(vga_cursor_t cursor);
vga_cursor_t vga_get_cursor(void);
void vga_putc(char ch);
void vga_write(const char *s);
void vga_write_ansi(const char *s);
void vga_write_hex(u32 value);

/* Writes text at an absolute screen position without changing global cursor. */
void vga_write_at(u8 row, u8 col, const char *s);
void vga_fill_row(u8 row, char ch);
