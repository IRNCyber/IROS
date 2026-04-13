# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog principles and semantic versioning.

---

## [v0.0.1] - Initial Release

### Added
- Bootable BIOS-based bootloader
- 32-bit protected mode transition
- Kernel entry and initialization logic
- Basic VGA text output (direct memory access)
- Initial kernel message: "IROS Kernel Initialized"
- Minimal memory allocator (`kmalloc`)
- Basic project structure (boot, kernel, include, tools)

### Technical Details
- Freestanding C kernel (no standard library)
- Custom linker script (`linker.ld`)
- Raw disk image generation (`iros.img`)
- Compatible with QEMU emulator

### Validation
- Successful boot in QEMU
- Kernel execution verified without crashes
- Memory allocation test:
  kmalloc(64) = 0x000016A0

### Limitations
- No keyboard input support
- No interrupt handling (IDT/IRQ)
- No interactive shell
- No multitasking or scheduling
- Static execution after initialization

### Notes
- This release establishes the foundational architecture of IROS
- Intended as a base for incremental OS development

---

[Full Changelog](https://github.com/IRNCyber/IROS/commits/v0.0.1)
