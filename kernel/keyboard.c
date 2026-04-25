#include <iros/isr.h>
#include <iros/keyboard.h>
#include <iros/mouse.h>
#include <iros/pic.h>
#include <iros/ports.h>
#include <iros/cpu.h>

enum { KBD_DATA = 0x60, KBD_STATUS = 0x64 };
enum { KBD_STATUS_OUT = 1u, KBD_STATUS_AUX = 0x20u };

static volatile u16 ring[128];
static volatile u32 ring_r = 0;
static volatile u32 ring_w = 0;
static u8 key_down[128];

static int shift_down = 0;
static int ctrl_down = 0;
static int caps_lock = 0;
static int e0_prefix = 0;
static int irq_enabled = 0;

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
  return shifted ? map_shift[sc] : map[sc];
}

static void keyboard_process_scancode(u8 sc) {
  if (sc == 0xE0) {
    e0_prefix = 1;
    return;
  }

  /* Handle releases. */
  if (sc & 0x80u) {
    u8 rel = (u8)(sc & 0x7Fu);
    if (rel < 128) key_down[rel] = 0;
    if (e0_prefix) {
      e0_prefix = 0;
      return;
    }
    if (rel == 0x2A || rel == 0x36) shift_down = 0;
    if (rel == 0x1D) ctrl_down = 0;
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

  /* Keypad navigation when NumLock is off can arrive as non-E0 Set 1 codes. */
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
    default: break;
  }

  /* Shift make */
  if (sc == 0x2A || sc == 0x36) {
    key_down[sc] = 1;
    shift_down = 1;
    return;
  }

  /* Ctrl make */
  if (sc == 0x1D) {
    key_down[sc] = 1;
    ctrl_down = 1;
    return;
  }

  /* Caps Lock toggle */
  if (sc == 0x3A) {
    caps_lock = !caps_lock;
    return;
  }

  /* Debounce typematic repeats: accept one make per press, until break arrives. */
  if (sc < 128) {
    if (key_down[sc]) return;
    key_down[sc] = 1;
  }

  /* Caps Lock only affects letter keys; check with base map first. */
  char base = scancode_to_ascii(sc, 0);
  int is_letter = (base >= 'a' && base <= 'z');
  int effective_shift = is_letter ? (shift_down ^ caps_lock) : shift_down;
  char c = scancode_to_ascii(sc, effective_shift);
  if (ctrl_down && c >= 'a' && c <= 'z') {
    ring_put((u16)(u8)(c - 'a' + 1));
  } else if (ctrl_down && c >= 'A' && c <= 'Z') {
    ring_put((u16)(u8)(c - 'A' + 1));
  } else if (c) {
    ring_put((u16)(u8)c);
  }
}

static void keyboard_poll_once(void) {
  u8 status = inb(KBD_STATUS);
  if ((status & KBD_STATUS_OUT) == 0) return;
  /* Drain AUX bytes and route them to mouse parser so keyboard cannot be starved. */
  if (status & KBD_STATUS_AUX) {
    u8 data = inb(KBD_DATA);
    mouse_feed_byte(data);
    mouse_flush_ui();
    return;
  }
  u8 sc = inb(KBD_DATA);
  keyboard_process_scancode(sc);
}

static void keyboard_irq(regs_t *r) {
  (void)r;

  u8 status = inb(KBD_STATUS);
  if ((status & KBD_STATUS_OUT) == 0) return;
  /* Drain stray AUX bytes on IRQ1 so controller output does not clog. */
  if (status & KBD_STATUS_AUX) {
    u8 data = inb(KBD_DATA);
    mouse_feed_byte(data);
    return;
  }

  u8 sc = inb(KBD_DATA);
  keyboard_process_scancode(sc);
}

void keyboard_init(void) {
  for (u32 i = 0; i < 128u; i++) key_down[i] = 0;
  ring_r = 0;
  ring_w = 0;
  shift_down = 0;
  ctrl_down = 0;
  caps_lock = 0;
  e0_prefix = 0;

  isr_register(33, keyboard_irq);

  /* Drain any pending bytes before enabling IRQs to avoid an immediate storm. */
  for (u32 i = 0; i < 32u; i++) {
    if ((inb(KBD_STATUS) & 1u) == 0) break;
    (void)inb(KBD_DATA);
  }

  pic_set_mask(1, 0);
  irq_enabled = 1;
}

char keyboard_try_getchar(void) {
  u16 k = ring_get();
  if (k >= 0x100) return 0;
  return (char)(u8)k;
}

char keyboard_getchar(void) {
  for (;;) {
    u16 k = keyboard_getkey();
    if (k && k < 0x100) return (char)(u8)k;
  }
}

u16 keyboard_try_getkey(void) {
  mouse_flush_ui();
  u16 k = ring_get();
  if (k) return k;
  keyboard_poll_once();
  mouse_flush_ui();
  return ring_get();
}

u16 keyboard_getkey(void) {
  for (;;) {
    mouse_flush_ui();
    u16 k = ring_get();
    if (k) return k;
    keyboard_poll_once();
    mouse_flush_ui();
    k = ring_get();
    if (k) return k;
    for (u32 i = 0; i < (irq_enabled ? 1000u : 10000u); i++) { __asm__ volatile("nop"); }
  }
}
