


# IROS (Intelligent Real-time Operating System)

Minimal BIOS-bootable 32-bit x86 kernel written in C (freestanding) with a tiny boot sector loader.

---

## Overview

This version (**v0.0.1**) represents the initial working kernel of IROS.
The system successfully boots, initializes core components, and demonstrates basic memory and output functionality.

---

## What it does (v0.0.1)

* BIOS boot sector loads the kernel from disk into memory
* Switches CPU to 32-bit protected mode
* Initializes a basic VGA text console
* Displays: `IROS Kernel Initialized`
* Implements a minimal memory allocator (`kmalloc`)
* Confirms memory allocation during runtime:

  ```
  kmalloc(64) = 0x000016A0
  ```

---

## Current Capabilities

* Bootable kernel image
* Stable execution in emulator (QEMU)
* Direct hardware-level screen output
* Basic memory allocation (bump allocator)

---

## Known Limitations

* No keyboard input support
* No interrupt handling (IDT/IRQ not implemented)
* No interactive shell or command processing
* No multitasking or scheduling
* Static execution after initialization

---

## Project Structure

* `boot/` – BIOS boot sector (Assembly + linker script)
* `kernel/` – kernel entry and core logic
* `include/` – freestanding headers
* `linker.ld` – kernel linker script
* `build.ps1` – Windows build script
* `build.cmd` – Windows shortcut build script
* `Makefile` – cross-platform build system
* `tools/` – helper scripts (image builder)

---

## Toolchain Requirements

### Option A (LLVM – Recommended)

* `clang`
* `ld.lld`
* `llvm-objcopy`

### Option B (GNU Cross Toolchain)

* `i686-elf-gcc`
* `i686-elf-ld`
* `i686-elf-objcopy`

### Additional Tools

* `python3`
* `qemu-system-i386`

---

## Build Instructions

### Windows (PowerShell)

```powershell
.\build.ps1
```

Alternative:

```bat
build.cmd
```

Run:

```powershell
qemu-system-i386 -drive format=raw,file=build\iros.img
```

---

### Linux / macOS

```sh
make
qemu-system-i386 -drive format=raw,file=build/iros.img
```

---

### Windows (Make – Optional)

```bat
mingw32-make CC=clang LD=ld.lld OBJCOPY=llvm-objcopy PY=python
```

If `ld.lld` is unavailable:

```bat
mingw32-make CC=clang LD=lld OBJCOPY=llvm-objcopy PY=python
```

If `mingw32-make` is not recognized:

* Use `build.ps1` or `build.cmd` (recommended)
* Or add MinGW `bin` directory to PATH

---

## Notes

* This is a foundational kernel (no libc, paging, or multitasking)
* Bootloader loads a fixed kernel binary from disk image
* Designed for learning and incremental OS development

---

## Roadmap (Next Milestones)

* Keyboard driver implementation
* Interrupt handling (IDT + IRQ)
* Interactive shell (CLI)
* Enhanced memory management
* Modular kernel architecture

---

## Summary

IROS v0.0.1 establishes the core foundation of the system, proving control over boot process, memory, and hardware-level output. Future versions will focus on interactivity, system stability, and expanded operating system capabilities.

---
