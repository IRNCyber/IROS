# IROS (Intelligent Real-time Operating System)

Minimal BIOS-bootable 32-bit x86 kernel written in C (freestanding) with a tiny boot sector loader.

## What it does (current)
- BIOS boot sector loads the kernel from disk into memory.
- Switches to 32-bit protected mode.
- Kernel initializes a simple VGA text console and prints: `IROS Kernel Initialized`.
- Provides a tiny bump allocator (`kmalloc`) for basic memory management.

## Layout
- `boot/` – BIOS boot sector (`.S` + linker script)
- `kernel/` – kernel entry + core (C + `.S`)
- `include/` – freestanding headers
- `linker.ld` – kernel link script
- `build.ps1` – Windows build (GNU i686-elf or LLVM toolchain + QEMU)
- `Makefile` – Make-based build (GNU i686-elf or LLVM toolchain + QEMU)
- `tools/` – helper scripts (image builder)

## Toolchain prerequisites
You need a 32-bit freestanding cross toolchain and an emulator.

Option A (LLVM, easiest on Windows):
- `clang`
- `ld.lld`
- `llvm-objcopy`

Option B (GNU cross toolchain):
- `i686-elf-gcc`
- `i686-elf-ld`
- `i686-elf-objcopy`

Also:
- `python3` (used by `Makefile` to build the raw image)
- `qemu-system-i386` (to run)

## Build (Windows PowerShell)
```powershell
.\build.ps1
```

If you don’t want to type PowerShell flags, you can also run:
```bat
build.cmd
```

Run:
```powershell
qemu-system-i386 -drive format=raw,file=build\iros.img
```

If QEMU isn’t on `PATH`, install it and re-open the terminal, or run it by full path.

## Build (Linux/macOS)
```sh
make
qemu-system-i386 -drive format=raw,file=build/iros.img
```

## Build (Make on Windows)
If you run `mingw32-make` from CLion/MinGW:
- Ensure an ELF-capable linker is on PATH (`ld.lld` from LLVM, or `i686-elf-ld`). MinGW’s default `ld` (PE/COFF) will fail with: `unrecognised emulation mode: elf_i386`.
- Ensure LLVM tools are on PATH (`clang`, `ld.lld`, `llvm-objcopy`), or override `CC/LD/OBJCOPY`.
- Ensure Python is available; if your command is `python.exe`, use `PY=python`.

Example:
```bat
mingw32-make CC=clang LD=ld.lld OBJCOPY=llvm-objcopy PY=python
```

If your LLVM install only provides `lld.exe` (not `ld.lld.exe`), use:
```bat
mingw32-make CC=clang LD=lld OBJCOPY=llvm-objcopy PY=python
```

If PowerShell says `mingw32-make` is not recognized, it simply isn’t on your `PATH` in that terminal.
Options:
- Use `.\build.ps1` / `build.cmd` instead (recommended).
- Or locate `mingw32-make.exe` (often inside CLion’s bundled MinGW) and add its `bin` directory to `PATH`, or run it via full path.

## Notes
- This is a teaching/foundation kernel: no libc, no paging, no multitasking yet.
- The bootloader currently reads a fixed kernel binary placed immediately after the boot sector in `build/iros.img`.
