#include <iros/isr.h>
#include <iros/pic.h>
#include <iros/vga.h>

static isr_handler_t handlers[256];

static const char *exception_names[] = {
  "Division Error",
  "Debug",
  "NMI",
  "Breakpoint",
  "Overflow",
  "Bound Range Exceeded",
  "Invalid Opcode",
  "Device Not Available",
  "Double Fault",
  "Coprocessor Segment Overrun",
  "Invalid TSS",
  "Segment Not Present",
  "Stack-Segment Fault",
  "General Protection Fault",
  "Page Fault",
  "Reserved",
  "x87 FP Exception",
  "Alignment Check",
  "Machine Check",
  "SIMD FP Exception",
  "Virtualization Exception",
  "Control Protection Exception",
};

static u32 read_cr2(void) {
  u32 cr2;
  __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
  return cr2;
}

void isr_register(u8 int_no, isr_handler_t handler) {
  handlers[int_no] = handler;
}

void isr_handler_c(regs_t *r) {
  if (r->int_no < 256 && handlers[r->int_no]) {
    handlers[r->int_no](r);
  } else {
    if (r->int_no < 32) {
      vga_set_color(12, 0);
      vga_write("Exception ");
      vga_write_hex(r->int_no);
      if (r->int_no < 22) {
        vga_write(": ");
        vga_write(exception_names[r->int_no]);
      }
      vga_write("\n");

      vga_write("  EIP=");
      vga_write_hex(r->eip);
      vga_write("  ERR=");
      vga_write_hex(r->err_code);
      if (r->int_no == 14) {
        vga_write("  CR2=");
        vga_write_hex(read_cr2());
      }
      vga_write("\n");

      vga_write("  EAX=");
      vga_write_hex(r->eax);
      vga_write("  EBX=");
      vga_write_hex(r->ebx);
      vga_write("  ECX=");
      vga_write_hex(r->ecx);
      vga_write("  EDX=");
      vga_write_hex(r->edx);
      vga_write("\n");

      vga_write("  ESI=");
      vga_write_hex(r->esi);
      vga_write("  EDI=");
      vga_write_hex(r->edi);
      vga_write("  EBP=");
      vga_write_hex(r->ebp);
      vga_write("  ESP=");
      vga_write_hex(r->esp);
      vga_write("\n");

      vga_set_color(15, 0);
    }
  }

  if (r->int_no >= 32 && r->int_no < 48) {
    pic_send_eoi((u8)(r->int_no - 32));
  }
}
