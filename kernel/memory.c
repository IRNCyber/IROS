#include <iros/memory.h>
#include <iros/vga.h>

extern u8 __kernel_end;

static usize heap_ptr = 0;
static usize heap_end = 0;

static inline usize align_up(usize value, usize alignment) {
  if (alignment == 0) return value;
  usize mask = alignment - 1;
  return (value + mask) & ~mask;
}

void kmem_init(void) {
  heap_ptr = align_up((usize)&__kernel_end, 16);
  heap_end = 0x80000; /* 512 KiB physical, enough for now */

  if (heap_ptr >= heap_end) {
    vga_write("kmem_init: heap overflow\n");
    for (;;) __asm__ volatile("hlt");
  }
}

void *kmalloc(usize size, usize alignment) {
  if (size == 0) return (void *)0;
  heap_ptr = align_up(heap_ptr, alignment ? alignment : 16);
  if (heap_ptr + size > heap_end) {
    return (void *)0;
  }
  void *p = (void *)heap_ptr;
  heap_ptr += size;
  return p;
}

