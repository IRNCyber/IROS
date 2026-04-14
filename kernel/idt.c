#include <iros/idt.h>
#include <iros/isr.h>

typedef struct __attribute__((packed)) {
  u16 base_low;
  u16 sel;
  u8 always0;
  u8 flags;
  u16 base_high;
} idt_entry_t;

typedef struct __attribute__((packed)) {
  u16 limit;
  u32 base;
} idt_ptr_t;

extern void idt_flush(u32 idt_ptr);

extern void isr_stub_0(void);
extern void isr_stub_1(void);
extern void isr_stub_2(void);
extern void isr_stub_3(void);
extern void isr_stub_4(void);
extern void isr_stub_5(void);
extern void isr_stub_6(void);
extern void isr_stub_7(void);
extern void isr_stub_8(void);
extern void isr_stub_9(void);
extern void isr_stub_10(void);
extern void isr_stub_11(void);
extern void isr_stub_12(void);
extern void isr_stub_13(void);
extern void isr_stub_14(void);
extern void isr_stub_15(void);
extern void isr_stub_16(void);
extern void isr_stub_17(void);
extern void isr_stub_18(void);
extern void isr_stub_19(void);
extern void isr_stub_20(void);
extern void isr_stub_21(void);
extern void isr_stub_22(void);
extern void isr_stub_23(void);
extern void isr_stub_24(void);
extern void isr_stub_25(void);
extern void isr_stub_26(void);
extern void isr_stub_27(void);
extern void isr_stub_28(void);
extern void isr_stub_29(void);
extern void isr_stub_30(void);
extern void isr_stub_31(void);
extern void isr_stub_32(void);
extern void isr_stub_33(void);
extern void isr_stub_34(void);
extern void isr_stub_35(void);
extern void isr_stub_36(void);
extern void isr_stub_37(void);
extern void isr_stub_38(void);
extern void isr_stub_39(void);
extern void isr_stub_40(void);
extern void isr_stub_41(void);
extern void isr_stub_42(void);
extern void isr_stub_43(void);
extern void isr_stub_44(void);
extern void isr_stub_45(void);
extern void isr_stub_46(void);
extern void isr_stub_47(void);
extern void isr_stub_255(void);

static idt_entry_t idt[256];
static idt_ptr_t idtp;

static void idt_set_gate(u8 num, u32 base, u16 sel, u8 flags) {
  idt[num].base_low = (u16)(base & 0xFFFF);
  idt[num].base_high = (u16)((base >> 16) & 0xFFFF);
  idt[num].sel = sel;
  idt[num].always0 = 0;
  idt[num].flags = flags;
}

void idt_init(void) {
  idtp.limit = (u16)(sizeof(idt_entry_t) * 256 - 1);
  idtp.base = (u32)(u32)(usize)&idt[0];

  for (u32 i = 0; i < 256; i++) {
    idt_set_gate((u8)i, (u32)(usize)isr_stub_255, 0x08, 0x8E);
  }

  void *stubs[] = {
      (void *)isr_stub_0,  (void *)isr_stub_1,  (void *)isr_stub_2,  (void *)isr_stub_3,
      (void *)isr_stub_4,  (void *)isr_stub_5,  (void *)isr_stub_6,  (void *)isr_stub_7,
      (void *)isr_stub_8,  (void *)isr_stub_9,  (void *)isr_stub_10, (void *)isr_stub_11,
      (void *)isr_stub_12, (void *)isr_stub_13, (void *)isr_stub_14, (void *)isr_stub_15,
      (void *)isr_stub_16, (void *)isr_stub_17, (void *)isr_stub_18, (void *)isr_stub_19,
      (void *)isr_stub_20, (void *)isr_stub_21, (void *)isr_stub_22, (void *)isr_stub_23,
      (void *)isr_stub_24, (void *)isr_stub_25, (void *)isr_stub_26, (void *)isr_stub_27,
      (void *)isr_stub_28, (void *)isr_stub_29, (void *)isr_stub_30, (void *)isr_stub_31,
      (void *)isr_stub_32, (void *)isr_stub_33, (void *)isr_stub_34, (void *)isr_stub_35,
      (void *)isr_stub_36, (void *)isr_stub_37, (void *)isr_stub_38, (void *)isr_stub_39,
      (void *)isr_stub_40, (void *)isr_stub_41, (void *)isr_stub_42, (void *)isr_stub_43,
      (void *)isr_stub_44, (void *)isr_stub_45, (void *)isr_stub_46, (void *)isr_stub_47,
  };

  for (u32 i = 0; i < 48; i++) {
    idt_set_gate((u8)i, (u32)(usize)stubs[i], 0x08, 0x8E);
  }

  /* Use an 'm' constraint + memory clobber so the compiler commits idtp to memory before lidt. */
  __asm__ volatile("lidt %0" : : "m"(idtp) : "memory");
}
