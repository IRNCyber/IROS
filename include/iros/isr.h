#pragma once

#include <iros/types.h>

typedef struct regs {
  u32 gs, fs, es, ds;
  u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
  u32 int_no, err_code;
  u32 eip, cs, eflags;
} regs_t;

typedef void (*isr_handler_t)(regs_t *r);

void isr_register(u8 int_no, isr_handler_t handler);
void isr_handler_c(regs_t *r);

