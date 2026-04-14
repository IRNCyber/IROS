#include <iros/ports.h>
#include <iros/serial.h>

enum {
  COM1 = 0x3F8,
};

static inline void com_out(u16 off, u8 v) { outb((u16)(COM1 + off), v); }
static inline u8 com_in(u16 off) { return inb((u16)(COM1 + off)); }

void serial_init(void) {
  /* Disable interrupts */
  com_out(1, 0x00);
  /* Enable DLAB */
  com_out(3, 0x80);
  /* Baud divisor (115200/1 = 115200) */
  com_out(0, 0x01);
  com_out(1, 0x00);
  /* 8 bits, no parity, one stop bit */
  com_out(3, 0x03);
  /* Enable FIFO, clear, 14-byte threshold */
  com_out(2, 0xC7);
  /* IRQs enabled, RTS/DSR set */
  com_out(4, 0x0B);
}

int serial_can_read(void) {
  return (com_in(5) & 0x01) != 0;
}

int serial_can_write(void) {
  return (com_in(5) & 0x20) != 0;
}

int serial_read_byte(void) {
  if (!serial_can_read()) return -1;
  return (int)com_in(0);
}

void serial_write_byte(u8 b) {
  while (!serial_can_write()) { }
  com_out(0, b);
}

void serial_write(const char *s) {
  for (u32 i = 0; s && s[i]; i++) {
    char c = s[i];
    if (c == '\n') serial_write_byte('\r');
    serial_write_byte((u8)c);
  }
}

