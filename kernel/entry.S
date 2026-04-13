/* 32-bit kernel entry (freestanding)
 * Built with the C preprocessor (file is .S).
 */

.section .text
.global _start
.extern kernel_main

_start:
  cli
  movl $0x90000, %esp
  call kernel_main

1:
  hlt
  jmp 1b

