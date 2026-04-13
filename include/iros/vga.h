#pragma once

#include <iros/types.h>

void vga_clear(void);
void vga_write(const char *s);
void vga_write_hex(u32 value);

