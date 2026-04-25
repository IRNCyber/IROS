.section .text
.code32

.global gdt_flush
gdt_flush:
  movl 4(%esp), %eax
  lgdt (%eax)

  movw $0x10, %ax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs
  movw %ax, %gs
  movw %ax, %ss

  /* Reload CS via far jump. */
  ljmp $0x08, $gdt_flush_done
gdt_flush_done:
  ret

