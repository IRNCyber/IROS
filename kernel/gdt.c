#include <iros/gdt.h>
#include <iros/types.h>

typedef struct __attribute__((packed)) {
  u16 limit_low;
  u16 base_low;
  u8 base_mid;
  u8 access;
  u8 granularity;
  u8 base_high;
} gdt_entry_t;

typedef struct __attribute__((packed)) {
  u16 limit;
  u32 base;
} gdt_ptr_t;

extern void gdt_flush(u32 gdt_ptr);

/* 6 entries: null, kernel code, kernel data, user code, user data, TSS */
static gdt_entry_t gdt[6];
static gdt_ptr_t gdtp;

static void gdt_set_gate(u32 num, u32 base, u32 limit, u8 access, u8 gran) {
  gdt[num].base_low = (u16)(base & 0xFFFFu);
  gdt[num].base_mid = (u8)((base >> 16) & 0xFFu);
  gdt[num].base_high = (u8)((base >> 24) & 0xFFu);

  gdt[num].limit_low = (u16)(limit & 0xFFFFu);
  gdt[num].granularity = (u8)((limit >> 16) & 0x0Fu);
  gdt[num].granularity |= (u8)(gran & 0xF0u);
  gdt[num].access = access;
}

void gdt_set_tss(u32 base, u32 limit) {
  gdt_set_gate(5, base, limit, 0x89u, 0x00u);
}

void gdt_init(void) {
  gdtp.limit = (u16)(sizeof(gdt_entry_t) * 6u - 1u);
  gdtp.base = (u32)(usize)&gdt[0];

  /* 0: Null */
  gdt_set_gate(0, 0, 0, 0, 0);
  /* 1: Kernel code (Ring 0) - selector 0x08 */
  gdt_set_gate(1, 0, 0xFFFFFu, 0x9Au, 0xCFu);
  /* 2: Kernel data (Ring 0) - selector 0x10 */
  gdt_set_gate(2, 0, 0xFFFFFu, 0x92u, 0xCFu);
  /* 3: User code (Ring 3) - selector 0x18 | RPL3 = 0x1B */
  gdt_set_gate(3, 0, 0xFFFFFu, 0xFAu, 0xCFu);
  /* 4: User data (Ring 3) - selector 0x20 | RPL3 = 0x23 */
  gdt_set_gate(4, 0, 0xFFFFFu, 0xF2u, 0xCFu);
  /* 5: TSS - will be filled by tss_init() */
  gdt_set_gate(5, 0, 0, 0, 0);

  gdt_flush((u32)(usize)&gdtp);
}
