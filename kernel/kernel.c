#include <iros/memory.h>
#include <iros/vga.h>

void kernel_main(void) {
  vga_clear();
  vga_write("IROS Kernel Initialized\n");

  kmem_init();
  void *p = kmalloc(64, 16);
  vga_write("kmalloc(64) = ");
  vga_write_hex((u32)(usize)p);
  vga_write("\n");

  for (;;) {
    __asm__ volatile("hlt");
  }
}

