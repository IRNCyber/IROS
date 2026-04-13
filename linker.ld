ENTRY(_start)

SECTIONS
{
  /* The boot sector loads us to physical 0x1000. */
  . = 0x1000;

  .text :
  {
    *(.text .text.*)
  }

  .rodata :
  {
    *(.rodata .rodata.*)
  }

  .data :
  {
    *(.data .data.*)
  }

  .bss :
  {
    *(COMMON)
    *(.bss .bss.*)
  }

  __kernel_end = .;

  /DISCARD/ :
  {
    *(.eh_frame*)
    *(.comment)
    *(.note*)
  }
}
