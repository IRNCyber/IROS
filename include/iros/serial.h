#pragma once

#include <iros/types.h>

void serial_init(void);
int serial_can_read(void);
int serial_can_write(void);
int serial_read_byte(void); /* returns 0-255, or -1 if none */
void serial_write_byte(u8 b);
void serial_write(const char *s);

