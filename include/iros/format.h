#pragma once

#include <iros/types.h>

/* Format u32 as decimal into buf. Returns number of chars written. */
u32 fmt_u32_dec(char *buf, u32 bufsize, u32 value);

/* Format u32 as hex (0xXXXXXXXX) into buf. Returns number of chars written. */
u32 fmt_u32_hex(char *buf, u32 bufsize, u32 value);

/* Append a string to buf at *pos, respecting bufsize. */
void fmt_append(char *buf, u32 bufsize, u32 *pos, const char *s);
