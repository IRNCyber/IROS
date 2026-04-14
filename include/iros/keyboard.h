#pragma once

#include <iros/types.h>

/* Initializes keyboard input (IRQ1). */
void keyboard_init(void);

/* Non-blocking: returns 0 if no key available. */
char keyboard_try_getchar(void);

/* Blocking: waits for a keypress and returns ASCII (or 0 for non-printables). */
char keyboard_getchar(void);

/* Key codes for non-ASCII keys (returned by keyboard_getkey/try_getkey). */
enum {
  KEY_NONE = 0,
  KEY_UP = 0x100,
  KEY_DOWN = 0x101,
  KEY_LEFT = 0x102,
  KEY_RIGHT = 0x103,
  KEY_PGUP = 0x104,
  KEY_PGDN = 0x105,
  KEY_HOME = 0x106,
  KEY_END = 0x107,
  KEY_DEL = 0x108,
};

/* Non-blocking: returns KEY_NONE if no key available. */
u16 keyboard_try_getkey(void);

/* Blocking: returns ASCII in low byte, or KEY_* for special keys. */
u16 keyboard_getkey(void);
