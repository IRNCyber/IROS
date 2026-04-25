# RELEASE_v2.0.0.2


---

## Bug Fixes

### 1. Status bar buffer overflow (`kernel/status.c:35-56`)
The compiler warns about writing past `buf[64]`. The hex-formatted string `"heap used 0xXXXXXXXX free 0xXXXXXXXX"` is exactly 42 chars + null = 43, so it fits — but the loop bounds don't convince the compiler. Increase the buffer to `96` or add an explicit `if (idx >= sizeof(buf)-1) break;` inside every loop to silence the warning cleanly.

### 2. Makefile fails on Linux due to `CC` defaulting to `gcc`
`CC ?= clang` on line 66 uses conditional assignment, but GNU Make's implicit rules pre-set `CC=cc`, so `?=` never triggers on stock Linux. The build then fails at the boot sector link because `gcc` produces x86-64 objects without `--target=i686-elf`. Fix: use `override CC := clang` on Linux (matching the Windows block), or detect the default and override it.

### 3. Redundant code in `kfree` (`kernel/memory.c:122-123`)
```c
usize payload = align_up((usize)(b + 1), (b->padding ? 16 : 16));
payload = (usize)(b + 1) + (usize)b->padding;
```
The first line is dead code — `payload` is immediately overwritten. Remove the `align_up` call.

---

## Shell UX Improvements

### 4. In-line cursor editing (Left/Right, Home/End on the command line)
Currently Left/Right arrows are not handled during line input, so you can't go back and fix a typo mid-command. Track a `cursor_pos` within the line buffer and implement insert/delete at that position. This is the single biggest usability improvement you could make.

### 5. Tab completion for command names
Match the typed prefix against the known command list (`help`, `clear`, `mem`, …) and auto-complete or list matches. Low-hanging fruit since the command list is a static `strcmp` chain in `handle_line`.

### 6. `Ctrl+C` to clear the current line
Add a check for ASCII 0x03 in the key loop. On receipt, discard the line buffer, print `^C`, and re-print the prompt. Prevents having to backspace through a long mistyped command.

### 7. `Ctrl+L` to clear the screen (like most terminals)
Map ASCII 0x0C to `vga_clear()` followed by re-printing the prompt.

---

## New Features (Medium Effort)

### 8. PIT timer (IRQ0) + `uptime` command
The PIT is never programmed, so there's no sense of time. Initialize channel 0 at ~100 Hz, increment a tick counter in the IRQ handler, and expose an `uptime` command that converts ticks to seconds. This also lets you replace the busy-wait timeout in `serial_read_to_eot` with a real timeout.

### 9. RTC clock + timestamps in `dmesg`
Read the CMOS RTC (ports 0x70/0x71) to get wall-clock time. Prefix each `log_emit` line with `[HH:MM:SS]`. Add a `date` or `time` shell command.

### 10. Exception names & register dump
`isr_handler_c` currently only prints the interrupt number for unhandled exceptions. Print the exception name (e.g., "Division Error", "Page Fault"), the saved `eip`, and for fault 14, read CR2 to show the faulting address. This makes debugging much easier.

### 11. Caps Lock & Ctrl key support in the keyboard driver
The scancode map ignores Caps Lock (scancode 0x3A) and Ctrl (0x1D). Track their state and apply to ASCII conversion. This is needed for Ctrl+C/Ctrl+L above and for general typing usability.

### 12. Memory stats in decimal
`cmd_mem` outputs everything in hex. Add decimal-formatted output (you already have `write_u32_dec` in `shell.c`) for friendlier reading, e.g., `used: 1024 bytes (0x00000400)`.

---

## Larger Features (Higher Effort)

### 13. Simple filesystem (ramdisk or FAT12 on the disk image)
Currently apps are compiled into the kernel binary. A small FAT12 or custom ramdisk would let you load/store data at runtime, which is the foundation for user-created files, config persistence, and true app loading.

### 14. Serial console input (headless mode)
Serial output works, but input is read-only-on-demand for the Python bridge. Accepting shell input from COM1 would allow headless QEMU operation (`-nographic`) and remote debugging.

### 15. User-ring (Ring 3) task support
The GDT only has kernel code/data segments. Adding user-mode segments, a TSS, and basic context switching would turn IROS from a "monitor" into a real multitasking kernel, even if rudimentary.

### 16. Paging / virtual memory
Currently everything runs in flat physical memory up to 512 KiB. Setting up page tables would enable memory protection, a larger address space, and is a prerequisite for user-mode isolation.

---

## Code Quality

### 17. Extract command dispatch into a table
`handle_line` is a long `if/else if` chain (25+ branches). Refactor to a `{ name, handler_fn, help_text }` table. This also makes tab completion trivial (iterate the table) and keeps `help` output in sync automatically.

### 18. Consolidate hex/decimal formatting utilities
`write_u32_dec` is in `shell.c`, `vga_write_hex` is in `vga.c`, and `status.c` has its own inline hex formatter. Move all formatting to a shared `lib/format.c` or `lib/print.c`.

### 19. Add a `Makefile` `run` target for Linux
The `run` target exists but doesn't pass `-display gtk` or `-display sdl`. Adding a `DISPLAY` option or auto-detecting headless vs GUI would help out-of-the-box Linux usage.

---

## Quick Wins (< 30 min each)
| # | Change | Files |
|---|--------|-------|
| 3 | Remove dead `align_up` in `kfree` | `kernel/memory.c` |
| 1 | Widen status bar buffer | `kernel/status.c` |
| 12 | Decimal in `cmd_mem` | `kernel/shell.c` |
| 6 | Ctrl+C clears line | `kernel/shell.c`, `kernel/keyboard.c` |
| 7 | Ctrl+L clears screen | `kernel/shell.c` |
| 17 | Command dispatch table | `kernel/shell.c` |
| 2 | Fix Makefile CC on Linux | `Makefile` |
