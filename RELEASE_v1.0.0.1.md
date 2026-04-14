## IROS v1.0.0.1 - Interactive Shell + Kali-Like Text UI + Apps (Host-Assisted Git)

### Overview
v1.0.0.1 upgrades IROS from a static boot demo into a usable interactive system: interrupts are live, keyboard input works, and the kernel boots into a command shell with a Kali-like text aesthetic.

---

## What’s Included (v1.0.0.1)

### OS Core
- BIOS boot to 32-bit protected mode kernel
- VGA text console:
  - scrolling
  - hardware cursor
  - backspace support
  - bottom status bar (heap used/free)
- Structured logging:
  - `log_info(...)`
  - `log_error(...)`

### Interrupts + Input
- IDT installed with ISR stubs
- PIC remapped to `0x20`/`0x28` with proper EOI handling
- IRQ1 keyboard driver:
  - reads scancodes from `0x60`/`0x64`
  - basic US Set 1 mapping to ASCII
  - buffered via a ring buffer

### Shell
Boots into an interactive prompt (Kali-like styled prompt in VGA text mode).

Core commands:
- `help`, `clear` / `cls`, `mem`, `echo <text>`
- `version`, `about`, `banner`
- `color <fg> <bg>`, `status on|off`, `prompt kali|simple`
- `alloc [size]`, `free`, `reboot`, `halt`

### Memory Management
- Heap allocator upgrade:
  - `kmalloc(size, alignment)`
  - `kfree(ptr)`
  - `kmem_stats()` (used by `mem` + status bar)

Note: the kernel is currently built with SSE/MMX/x87 disabled (soft-float) to avoid invalid-opcode faults before proper FPU/SSE init is implemented.

### Apps (Compiled-In Registry)
This release adds a simple app system compiled into the kernel:
- `app list`
- `app info <name>`
- `app run <name>`
- `app hostrun <name>` (prints host run command)

Example app:
- `hello` (shows basic text output)

---

## Installing Apps (Git, Host-Assisted)
IROS does not run full git on bare metal yet. App installation is done on the host:

```powershell
powershell -ExecutionPolicy Bypass -File tools\iros-install.ps1 https://github.com/<user>/<repo>
.\build.ps1
.\run.ps1
```

Shell helper:
- `app install <git-url>` and `git install <git-url>` print the host install command.
- `app hostrun <name>` prints the host command to execute Python apps via `tools/iros-hostrun.ps1`.

---

## Build & Run (Windows)

Build:
```powershell
.\build.ps1
```

Run (QEMU):
```powershell
.\run.ps1
```

If `run.ps1` can’t find QEMU, set:
```powershell
$env:QEMU_HOME = "C:\Program Files\qemu"
```

---

# Phase 2: Kali-Like UI Implementation Plan

## Approach A: Kali-Themed Text UI (Implemented in v1.0.0.1)
- Minimal ANSI SGR color support (`\x1b[...m`)
- Styled prompt
- Bottom status bar

## Approach B: Full Graphical Desktop (Future)
If you want a GUI-like Kali desktop, the next large steps are:
1) VESA/VBE graphics mode + linear framebuffer
2) 2D rendering primitives
3) Bitmap font + terminal renderer
4) PS/2 mouse (IRQ12)
5) Simple compositor + theming
