#include <iros/isr.h>
#include <iros/mouse.h>
#include <iros/pic.h>
#include <iros/ports.h>
#include <iros/vga.h>

enum {
  PS2_DATA = 0x60,
  PS2_STATUS = 0x64,
  PS2_COMMAND = 0x64,
  PS2_STATUS_OUT = 1,
  PS2_STATUS_IN = 2,
  PS2_STATUS_AUX = 0x20
};

static volatile u8 mouse_row = 12;
static volatile u8 mouse_col = 39;
static volatile u8 mouse_buttons = 0;
static volatile u8 mouse_enabled = 0;
static volatile u8 mouse_dirty = 0;

static u8 packet[3];
static u8 packet_index = 0;
static i32 accum_dx = 0;
static i32 accum_dy = 0;

static int ps2_wait_input_clear(void) {
  for (u32 i = 0; i < 100000u; i++) {
    if ((inb(PS2_STATUS) & PS2_STATUS_IN) == 0) return 1;
  }
  return 0;
}

static int ps2_wait_output_full(void) {
  for (u32 i = 0; i < 100000u; i++) {
    if (inb(PS2_STATUS) & PS2_STATUS_OUT) return 1;
  }
  return 0;
}

static int ps2_read_byte(u8 *out) {
  if (!ps2_wait_output_full()) return 0;
  *out = inb(PS2_DATA);
  return 1;
}

static int ps2_write_command(u8 cmd) {
  if (!ps2_wait_input_clear()) return 0;
  outb(PS2_COMMAND, cmd);
  return 1;
}

static int ps2_write_data(u8 data) {
  if (!ps2_wait_input_clear()) return 0;
  outb(PS2_DATA, data);
  return 1;
}

static int mouse_write(u8 value) {
  if (!ps2_write_command(0xD4)) return 0;
  if (!ps2_write_data(value)) return 0;
  return 1;
}

static int mouse_expect_ack(void) {
  u8 b = 0;
  for (u32 i = 0; i < 100000u; i++) {
    if (!ps2_read_byte(&b)) continue;
    if (b == 0xFA) return 1;
    if (b == 0xFE) return 0;
  }
  return 0;
}

static void mouse_apply_movement(i32 dx, i32 dy) {
  accum_dx += dx;
  accum_dy += dy;

  while (accum_dx >= 2) {
    if (mouse_col < 78) mouse_col++;
    accum_dx -= 2;
  }
  while (accum_dx <= -2) {
    if (mouse_col > 0) mouse_col--;
    accum_dx += 2;
  }

  while (accum_dy >= 2) {
    if (mouse_row > 0) mouse_row--;
    accum_dy -= 2;
  }
  while (accum_dy <= -2) {
    if (mouse_row < 23) mouse_row++;
    accum_dy += 2;
  }
}

static void mouse_process_packet(void) {
  if ((packet[0] & 0x08u) == 0) return;
  if (packet[0] & 0xC0u) return; /* overflow */

  i32 dx = (i32)(i8)packet[1];
  i32 dy = (i32)(i8)packet[2];
  mouse_buttons = (u8)(packet[0] & 0x07u);

  mouse_apply_movement(dx, dy);
  mouse_dirty = 1;
}

void mouse_feed_byte(u8 data) {
  packet[packet_index++] = data;
  if (packet_index == 1 && (packet[0] & 0x08u) == 0) {
    packet_index = 0;
    return;
  }
  if (packet_index >= 3) {
    packet_index = 0;
    mouse_process_packet();
  }
}

void mouse_poll_once(void) {
  u8 status = inb(PS2_STATUS);
  if ((status & PS2_STATUS_OUT) == 0) return;
  if ((status & PS2_STATUS_AUX) == 0) return;

  u8 data = inb(PS2_DATA);
  mouse_feed_byte(data);
}

static void mouse_irq(regs_t *r) {
  (void)r;
  for (;;) {
    u8 status = inb(PS2_STATUS);
    if ((status & PS2_STATUS_OUT) == 0) break;
    if ((status & PS2_STATUS_AUX) == 0) break;
    mouse_feed_byte(inb(PS2_DATA));
  }
}

void mouse_init(void) {
  isr_register(44, mouse_irq);

  if (!ps2_write_command(0xA8)) return; /* enable aux device */

  if (!ps2_write_command(0x20)) return; /* read controller command byte */
  u8 command_byte = 0;
  if (!ps2_read_byte(&command_byte)) return;
  command_byte |= 0x03u; /* enable IRQ1 + IRQ12 */
  command_byte &= (u8)~0x20u; /* enable mouse clock */
  if (!ps2_write_command(0x60)) return;
  if (!ps2_write_data(command_byte)) return;

  /* Initialize mouse for stream mode. */
  if (!mouse_write(0xF6) || !mouse_expect_ack()) return; /* defaults */
  if (!mouse_write(0xF4) || !mouse_expect_ack()) return; /* enable data reporting */

  packet_index = 0;
  accum_dx = 0;
  accum_dy = 0;
  mouse_enabled = 1;
  mouse_dirty = 1;
  pic_set_mask(12, 0);

  vga_pointer_enable(1);
  vga_pointer_set(mouse_row, mouse_col);
}

void mouse_set_enabled(int enabled) {
  if (enabled) {
    if (!mouse_enabled) return;
    mouse_dirty = 1;
    vga_pointer_enable(1);
    vga_pointer_set(mouse_row, mouse_col);
  } else {
    mouse_dirty = 0;
    vga_pointer_enable(0);
  }
}

int mouse_is_enabled(void) {
  return mouse_enabled && vga_pointer_is_enabled();
}

void mouse_set_position(u8 row, u8 col) {
  if (row > 23) row = 23;
  if (col > 78) col = 78;
  mouse_row = row;
  mouse_col = col;
  mouse_dirty = 1;
  if (mouse_enabled) vga_pointer_set(mouse_row, mouse_col);
}

mouse_state_t mouse_get_state(void) {
  mouse_state_t s;
  s.row = mouse_row;
  s.col = mouse_col;
  s.buttons = mouse_buttons;
  s.enabled = (u8)(mouse_is_enabled() ? 1 : 0);
  return s;
}

void mouse_flush_ui(void) {
  if (!mouse_enabled) return;
  vga_pointer_input(mouse_row, mouse_col, mouse_buttons);
  if (!mouse_dirty) return;
  mouse_dirty = 0;
  vga_pointer_set(mouse_row, mouse_col);
}
