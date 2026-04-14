#include <iros/pic.h>
#include <iros/ports.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

static u8 pic_read_mask(u16 data_port) {
  return inb((u16)data_port);
}

static void pic_write_mask(u16 data_port, u8 mask) {
  outb((u16)data_port, mask);
}

void pic_remap(u8 offset1, u8 offset2) {
  u8 a1 = pic_read_mask(PIC1_DATA);
  u8 a2 = pic_read_mask(PIC2_DATA);

  outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();

  outb(PIC1_DATA, offset1);
  io_wait();
  outb(PIC2_DATA, offset2);
  io_wait();

  outb(PIC1_DATA, 4);
  io_wait();
  outb(PIC2_DATA, 2);
  io_wait();

  outb(PIC1_DATA, ICW4_8086);
  io_wait();
  outb(PIC2_DATA, ICW4_8086);
  io_wait();

  pic_write_mask(PIC1_DATA, a1);
  pic_write_mask(PIC2_DATA, a2);
}

void pic_send_eoi(u8 irq) {
  if (irq >= 8) {
    outb(PIC2_COMMAND, 0x20);
  }
  outb(PIC1_COMMAND, 0x20);
}

void pic_set_mask(u8 irq, int masked) {
  u16 port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
  u8 mask = pic_read_mask(port);
  u8 bit = (u8)(1u << (irq & 7u));

  if (masked) mask |= bit;
  else mask &= (u8)~bit;

  pic_write_mask(port, mask);
}

