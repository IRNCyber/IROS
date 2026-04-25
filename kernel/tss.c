#include <iros/tss.h>

static tss_entry_t tss;

extern void gdt_set_tss(u32 base, u32 limit);

static void tss_flush(void) {
  /* TSS segment is at GDT index 5 => selector 0x28, but with RPL=0 => 0x2B for Ring 3 use. */
  __asm__ volatile("ltr %%ax" : : "a"((u16)0x28));
}

void tss_init(u32 kernel_ss, u32 kernel_esp) {
  u8 *p = (u8 *)&tss;
  for (u32 i = 0; i < sizeof(tss); i++) p[i] = 0;

  tss.ss0 = kernel_ss;
  tss.esp0 = kernel_esp;
  tss.iomap_base = (u16)sizeof(tss_entry_t);

  gdt_set_tss((u32)(usize)&tss, sizeof(tss_entry_t) - 1);
  tss_flush();
}

void tss_set_kernel_stack(u32 esp) {
  tss.esp0 = esp;
}
