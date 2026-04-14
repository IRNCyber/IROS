#include <iros/isr.h>
#include <iros/keyboard.h>
#include <iros/pic.h>
#include <iros/ports.h>
#include <iros/cpu.h>

enum { KBD_DATA = 0x60, KBD_STATUS = 0x64 };

static volatile u16 ring[128];
static volatile u32 ring_r = 0;
static volatile u32 ring_w = 0;

static int shift_down = 0;
static int e0_prefix = 0;

static void ring_put(u16 c) {
  u32 next = (ring_w + 1) & 127u;
  if (next == ring_r) return;
  ring[ring_w] = c;
  ring_w = next;
}

static u16 ring_get(void) {
  if (ring_r == ring_w) return 0;
  u16 c = ring[ring_r];
  ring_r = (ring_r + 1) & 127u;
  return c;
}

static char scancode_to_ascii(u8 sc, int shifted) {
  /* US QWERTY, Set 1 make codes only (no E0 extended). */
  static const char map[128] = {
      0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
      '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
      'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,  '\\',
      'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,  ' ',
  };
  static const char map_shift[128] = {
      0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
      '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
      'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,  '|',
      'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,  ' ',
  };

  if (sc >= 128) return 0;
  char c = shifted ? map_shift[sc] : map[sc];
  return c;
}

static void keyboard_irq(regs_t *r) {
  (void)r;

  u8 status = inb(KBD_STATUS);
  if ((status & 1u) == 0) return;

  u8 sc = inb(KBD_DATA);

  if (sc == 0xE0) {
    e0_prefix = 1;
    return;
  }

  /* Handle releases. */
  if (sc & 0x80u) {
    u8 rel = (u8)(sc & 0x7Fu);
    if (e0_prefix) {
      e0_prefix = 0;
      return;
    }
    if (rel == 0x2A || rel == 0x36) shift_down = 0;
    return;
  }

  if (e0_prefix) {
    e0_prefix = 0;
    /* Extended keys (Set 1, E0 xx). */
    switch (sc) {
      case 0x48: ring_put(KEY_UP); return;
      case 0x50: ring_put(KEY_DOWN); return;
      case 0x4B: ring_put(KEY_LEFT); return;
      case 0x4D: ring_put(KEY_RIGHT); return;
      case 0x49: ring_put(KEY_PGUP); return;
      case 0x51: ring_put(KEY_PGDN); return;
      case 0x47: ring_put(KEY_HOME); return;
      case 0x4F: ring_put(KEY_END); return;
      case 0x53: ring_put(KEY_DEL); return;
      default: return;
    }
  }

  /* Shift make */
  if (sc == 0x2A || sc == 0x36) {
    shift_down = 1;
    return;
  }

  char c = scancode_to_ascii(sc, shift_down);
  if (c) ring_put((u16)(u8)c);
}

void keyboard_init(void) {
  /* Ensure PIC IRQ1 is unmasked. */
  pic_set_mask(1, 0);

  /* IRQ1 after PIC remap to 0x20 is int 0x21 (33). */
  isr_register(33, keyboard_irq);
}

char keyboard_try_getchar(void) {
  u16 k = ring_get();
  if (k >= 0x100) return 0;
  return (char)(u8)k;
}

char keyboard_getchar(void) {
  for (;;) {
    u16 k = ring_get();
    if (k && k < 0x100) return (char)(u8)k;
    cpu_halt();
  }
}

u16 keyboard_try_getkey(void) {
  return ring_get();
}

u16 keyboard_getkey(void) {
  for (;;) {
    u16 k = ring_get();
    if (k) return k;
    cpu_halt();
  }
}
