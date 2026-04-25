#include <iros/cpu.h>
#include <iros/gdt.h>
#include <iros/idt.h>
#include <iros/keyboard.h>
#include <iros/log.h>
#include <iros/memory.h>
#include <iros/mouse.h>
#include <iros/paging.h>
#include <iros/pic.h>
#include <iros/ramdisk.h>
#include <iros/serial.h>
#include <iros/shell.h>
#include <iros/status.h>
#include <iros/timer.h>
#include <iros/tss.h>
#include <iros/vga.h>

static u8 kernel_stack[4096] __attribute__((aligned(16)));

void kernel_main(void) {
  cpu_cli();

  gdt_init();

  vga_init();
  log_init();
  log_info("IROS Kernel Initialized");

  kmem_init();
  status_init();

  serial_init();

  idt_init();
  pic_remap(0x20, 0x28);

  /* Mask everything by default to avoid stray IRQ spam; enable keyboard only. */
  for (u8 irq = 0; irq < 16; irq++) {
    pic_set_mask(irq, 1);
  }

  tss_init(0x10, (u32)(usize)&kernel_stack[sizeof(kernel_stack)]);
  paging_init();
  ramdisk_init();

  timer_init();
  keyboard_init();
  mouse_init();

  cpu_sti();

  shell_run();
}
