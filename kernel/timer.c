#include <iros/isr.h>
#include <iros/pic.h>
#include <iros/ports.h>
#include <iros/timer.h>

enum { PIT_CH0 = 0x40, PIT_CMD = 0x43 };
enum { PIT_HZ = 100 };

static volatile u32 ticks = 0;

static void timer_irq(regs_t *r) {
  (void)r;
  ticks++;
}

void timer_init(void) {
  /* Channel 0, lo/hi byte, rate generator (mode 2). */
  outb(PIT_CMD, 0x34);

  u16 divisor = (u16)(1193182u / PIT_HZ);
  outb(PIT_CH0, (u8)(divisor & 0xFF));
  outb(PIT_CH0, (u8)((divisor >> 8) & 0xFF));

  isr_register(32, timer_irq);
  pic_set_mask(0, 0);
}

u32 timer_get_ticks(void) {
  return ticks;
}

u32 timer_get_seconds(void) {
  return ticks / PIT_HZ;
}
