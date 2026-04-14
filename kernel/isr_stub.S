.section .text
.code32

.global idt_flush
idt_flush:
  movl 4(%esp), %eax
  lidt (%eax)
  ret

.extern isr_handler_c

.macro ISR_NOERR num
  .global isr_stub_\num
isr_stub_\num:
  pushl $0
  pushl $\num
  jmp isr_common
.endm

.macro ISR_ERR num
  .global isr_stub_\num
isr_stub_\num:
  pushl $\num
  jmp isr_common_err
.endm

/* Common handler for no-error-code interrupts (we already pushed 0 + int_no). */
isr_common:
  pusha
  push %ds
  push %es
  push %fs
  push %gs

  mov $0x10, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs

  pushl %esp
  call isr_handler_c
  add $4, %esp

  pop %gs
  pop %fs
  pop %es
  pop %ds
  popa
  add $8, %esp
  iret

/* Common handler for error-code interrupts (CPU pushed error code; stack has: err_code, int_no). */
isr_common_err:
  pusha
  push %ds
  push %es
  push %fs
  push %gs

  mov $0x10, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs

  pushl %esp
  call isr_handler_c
  add $4, %esp

  pop %gs
  pop %fs
  pop %es
  pop %ds
  popa
  add $4, %esp /* pop int_no, leave err_code? */
  add $4, %esp /* pop err_code */
  iret

/* Exceptions 0-31 */
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30
ISR_NOERR 31

/* IRQs after PIC remap to 32-47 */
ISR_NOERR 32
ISR_NOERR 33
ISR_NOERR 34
ISR_NOERR 35
ISR_NOERR 36
ISR_NOERR 37
ISR_NOERR 38
ISR_NOERR 39
ISR_NOERR 40
ISR_NOERR 41
ISR_NOERR 42
ISR_NOERR 43
ISR_NOERR 44
ISR_NOERR 45
ISR_NOERR 46
ISR_NOERR 47

/* Spurious/default handler. */
ISR_NOERR 255
