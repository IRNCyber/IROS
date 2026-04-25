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

---

## [v1.0.0.1] - 2026-04-14

### Added
- Interactive shell with commands: `help`, `clear`, `mem`, `echo`
- Keyboard driver via IRQ1 (PS/2 controller ports `0x60`/`0x64`) with basic US Set 1 scancode mapping
- IDT setup and ISR stubs; PIC remap to `0x20`/`0x28` with proper EOI handling
- Structured logging (`log_info`, `log_error`)
- VGA cursor control (hardware cursor) + backspace handling
- Heap allocator upgrade: `kmalloc` + `kfree` + `kmem_stats`
- Windows runner helper `run.ps1` (auto-detects QEMU via PATH/common install paths)

### Changed
- Build system no longer depends on NASM: boot sector and kernel entry use `.S` (GAS/Clang-compatible)
- Boot image generation is now cross-platform via `tools/mkimg.py`

### Validation
- Kernel builds to a raw bootable image (`build/iros.img`) via `build.ps1`
- Boot + shell validated under QEMU (requires QEMU installed and accessible)

### Limitations
- No multitasking/scheduler
- No paging/virtual memory
- Minimal keyboard support (no extended scancodes)

[Full Changelog](https://github.com/IRNCyber/IROS/commits/v1.0.0.1)

---

## [v1.0.0.2] - 2026-04-25

### Added
- Shell command history scrolling (Up/Down arrows)
- Kernel log ring buffer + `dmesg` command
- `log serial on|off` to mirror logs to COM1
- Python bridge (QEMU serial TCP) with `py ping|exec|run`
- GitHub Actions workflow to build `build/iros.img` artifact

### Changed
- App manifests support `type=` and `entry=`; Python apps can be executed via host bridge

### Notes
- Python is executed on the host and streamed back via serial; not embedded in the kernel yet

[Full Changelog](https://github.com/IRNCyber/IROS/commits/v1.0.0.2)
