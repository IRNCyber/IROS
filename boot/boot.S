/* IROS BIOS boot sector (ELF -> flat binary)
 * - Loads kernel sectors from LBA 1 into physical 0x00001000
 * - Switches to 32-bit protected mode
 * - Jumps to kernel entry at 0x1000
 *
 * Build notes:
 *   - Assemble with a freestanding i386-elf toolchain (clang --target=i686-elf works)
 *   - Link at 0x7C00 via boot/boot.ld
 *   - Objcopy to raw binary, must be exactly 512 bytes with 0xAA55 signature
 *
 * KERNEL_SECTORS is provided by the build system (number of 512-byte sectors in kernel.bin).
 */

#ifndef KERNEL_SECTORS
#define KERNEL_SECTORS 32
#endif

.section .text
.code16
.global _start

_start:
  cli
  xorw %ax, %ax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %ss
  movw $0x7C00, %sp
  sti

  movb %dl, boot_drive

  /* Check Int13 Extensions (EDD) */
  movw $0x4100, %ax
  movw $0x55AA, %bx
  movb boot_drive, %dl
  int $0x13
  jc disk_error
  cmpw $0xAA55, %bx
  jne disk_error

  /* Fill DAP and read kernel via LBA into 0000:1000 */
  movw $dap, %si

  movw $KERNEL_SECTORS, dap_sector_count
  movw $0x1000, dap_offset
  movw $0x0000, dap_segment
  movl $1, dap_lba_low
  movl $0, dap_lba_high

  movb $0x42, %ah
  movb boot_drive, %dl
  int $0x13
  jc disk_error

  cli
  call enable_a20_fast
  lgdt gdt_descriptor

  movl %cr0, %eax
  orl $0x1, %eax
  movl %eax, %cr0

  ljmp $0x08, $protected_mode_entry

disk_error:
  cli
1:
  hlt
  jmp 1b

enable_a20_fast:
  inb $0x92, %al
  orb $0x02, %al
  outb %al, $0x92
  ret

.code32
protected_mode_entry:
  movw $0x10, %ax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs
  movw %ax, %gs
  movw %ax, %ss

  movl $0x1000, %eax
  jmp *%eax

/* Disk Address Packet (DAP) for int 13h AH=42h */
.align 2
dap:
  .byte 0x10              /* size */
  .byte 0x00              /* reserved */
dap_sector_count:
  .word 0x0000            /* sector count (patched) */
dap_offset:
  .word 0x0000            /* offset (patched) */
dap_segment:
  .word 0x0000            /* segment (patched) */
dap_lba_low:
  .long 0x00000000        /* LBA low (patched) */
dap_lba_high:
  .long 0x00000000        /* LBA high */

boot_drive:
  .byte 0

/* Minimal GDT: null, code, data (flat) */
.align 8
gdt_start:
gdt_null:
  .quad 0x0000000000000000
gdt_code:
  .quad 0x00CF9A000000FFFF
gdt_data:
  .quad 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
  .word (gdt_end - gdt_start - 1)
  .long gdt_start

/* Pad to 512 and add signature. (Offset 510 from start of boot sector.) */
.org 510, 0
.word 0xAA55

