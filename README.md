# IROS (Intelligent Real-time Operating System)

Minimal BIOS-bootable 32-bit x86 kernel written in freestanding C with a tiny boot sector loader.

## Current Features
- BIOS boot sector loads the kernel from disk into memory
- Switches CPU to 32-bit protected mode
- VGA text console with cursor control, scrolling, and a bottom status bar
- Kali-like text UI (ANSI SGR colors + styled prompt in VGA text mode)
- Structured logging (`log_info`, `log_error`)
- Heap allocator (`kmalloc` / `kfree`) with usage stats (`mem` command)
- IDT + PIC remap + keyboard IRQ1 handler
- PS/2 mouse support (IRQ12) with text-mode pointer overlay
- Interactive shell with built-in commands
- Simple “apps” registry compiled into the kernel (`app list/info/run`)
- Project logo asset at `assets/iros-logo.svg`

## Layout
- `boot/` - BIOS boot sector (`boot.S` + `boot.ld`)
- `kernel/` - kernel code (C + `.S`)
- `include/` - freestanding headers
- `lib/` - small libc-like helpers (freestanding)
- `apps/` - app manifests (`*.app`) compiled into the kernel
- `tools/` - helper scripts (image builder + app codegen + host installer)
- `linker.ld` - kernel linker script
- `build.ps1` / `build.cmd` - Windows build
- `run.ps1` - Windows QEMU runner
- `Makefile` - Make-based build

## Toolchain
You need a 32-bit freestanding toolchain and an emulator.

Option A (LLVM, recommended on Windows):
- `clang`
- `ld.lld` (or `lld`)
- `llvm-objcopy`

Option B (GNU cross toolchain):
- `i686-elf-gcc`
- `i686-elf-ld`
- `i686-elf-objcopy`

Also:
- `python` / `python3` (used by `build.ps1` / `Makefile` / tools)
- `qemu-system-i386` (to run)

Note: the kernel is built with SSE/MMX/x87 disabled (soft-float) to avoid invalid-opcode faults before FPU/SSE init.

## Install Tools (Windows)
Pick one package manager path.

### Option 1: winget
```powershell
winget install --id LLVM.LLVM
winget install --id QEMU.QEMU
winget install --id Python.Python.3
winget install --id Git.Git
```

### Option 2: Chocolatey
```powershell
choco install -y llvm qemu python git
```

After installing, open a new terminal and verify:
```powershell
clang --version
ld.lld --version
llvm-objcopy --version
qemu-system-i386 --version
python --version
git --version
```

## Build + Run (Windows PowerShell)
```powershell
cd C:\Users\rohan\CLionProjects\IROS
.\build.ps1
.\run.ps1
```

IROS VM wrapper (IROS-branded launcher):
```powershell
.\iros-vm.ps1 -Build -Display sdl
```

IROS Emulator launcher (preferred name):
```powershell
.\iros-emulator.ps1 -Build -Display sdl
```
```bat
iros-emulator.cmd -Build -Display sdl
```

If `run.ps1` can’t find QEMU, set `QEMU_HOME` (adjust if installed elsewhere):
```powershell
$env:QEMU_HOME = "C:\Program Files\qemu"
```

## Shell Commands
Core:
- `help`, `clear` / `cls`, `mem`, `echo <text>`
- `dmesg`, `log serial on|off`
- `version`, `about`, `banner`, `logo`
- `color <fg> <bg>` (0-15), `status on|off`, `prompt kali|simple`, `history`, `keys`
- `ui show|kali|simple|panel`, `desktop`
- `mouse status|on|off|pos <row> <col>`
  - Left-click and drag on the right scrollbar to scroll up/down
- `alloc [size]`, `free`, `reboot`, `halt`

Apps:
- `app list`
- `app info <name>`
- `app run <name>`
- `app hostrun <name>` (host-side execution helper)
- `app install <git-url>` (prints host command)
- `git install <git-url>` (alias/help for the host installer)

Python bridge (runs Python on the host via QEMU serial TCP):
- Start QEMU with `-PyBridge` and run `tools/iros-pybridge.py` on the host
- In IROS:
  - `py ping`
  - `py exec <code>`
  - `py run <app-name>` (runs `apps-src/<app-name>` on the host; manifest `type=python` is recommended but not required)

Quick demo:
```text
help
banner
app list
app run hello
mem
echo hello from iros
```

## Installing Apps (Git, Host-Assisted)
IROS doesn’t run full git on bare metal yet (no network/filesystem). “Installing” an app is done on the host by cloning a repo and converting it into an `apps-installed/*.app` manifest, then regenerating `kernel/apps_gen.c`.

```powershell
powershell -ExecutionPolicy Bypass -File tools\iros-install.ps1 https://github.com/<user>/<repo>
tools\iros-hostrun.ps1 <appname> [entry.py]
.\build.ps1
.\run.ps1
```

## Running Python “from IROS” (Host Bridge)
1) Start QEMU with the bridge enabled:
```powershell
.\run.ps1 -PyBridge
```

2) In another terminal, connect the bridge:
```powershell
python tools\iros-pybridge.py
```

3) In IROS:
```text
py ping
py exec print("hello from python")
py run tactages
```

Base/distribution apps live in `apps/`. Host-installed apps live in `apps-installed/` (ignored by git).

If the repo contains `iros.app` at its root, it will be used directly. Otherwise, the installer will create a basic `.app` from the repo name and README snippet.

## Build (Make)
Linux/macOS:
```sh
make
qemu-system-i386 -drive format=raw,file=build/iros.img
```

Windows (if you have `mingw32-make` on PATH):
```bat
mingw32-make CC=clang LD=ld.lld OBJCOPY=llvm-objcopy PY=python
```

## Logo
- SVG logo file: `assets/iros-logo.svg`
- In-kernel command: `logo`
- Current kernel display is VGA text mode, so SVG rendering is host-side branding.

## Input Troubleshooting (QEMU)
- Click inside the QEMU window to capture keyboard input.
- Press `Ctrl+Alt+G` to release/reacquire input grab.
- Run in foreground while debugging input:
  - `.\run.ps1 -Detach:$false -Display sdl`
