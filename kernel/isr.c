#include <iros/isr.h>
#include <iros/pic.h>
#include <iros/vga.h>

static isr_handler_t handlers[256];

void isr_register(u8 int_no, isr_handler_t handler) {
  handlers[int_no] = handler;
}

void isr_handler_c(regs_t *r) {
  if (r->int_no < 256 && handlers[r->int_no]) {
    handlers[r->int_no](r);
  } else {
    /* Don't spam the console for unhandled IRQs (e.g. timer). */
    if (r->int_no < 32) {
      vga_set_color(12, 0);
      vga_write("Unhandled exception: ");
      vga_write_hex(r->int_no);
      vga_write("\n");
      vga_set_color(15, 0);
    }
  }

  if (r->int_no >= 32 && r->int_no < 48) {
    pic_send_eoi((u8)(r->int_no - 32));
  }
}
