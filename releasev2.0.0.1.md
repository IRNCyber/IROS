# IROS v2.0.0.1 — Interactive Core Upgrade

Release date: 2026-04-25

This release upgrades IROS from a minimal boot shell to a more complete interactive kernel experience with stronger input handling, richer shell controls, and improved run/build tooling.

---

## Highlights

### 1) Input Stability (Keyboard + Mouse)
- Hardened PS/2 input path for shared controller data (`0x60`/`0x64`)
- Improved AUX-vs-keyboard byte routing to reduce key duplication and missed input
- Stabilized mouse pointer overlay flow with safer redraw/update sequencing
- Improved shell navigation behavior for scrolling and history movement

### 2) UI / UX Improvements
- Added UI mode controls:
  - `ui show`
  - `ui kali`
  - `ui simple`
  - `ui panel`
- Added `desktop` view command
- Added/updated shell visuals:
  - Kali-like prompt option
  - status bar controls (`status on|off`)
  - color controls (`color <fg> <bg>`)
  - startup/info logging improvements
- Added IROS logo asset: `assets/iros-logo.svg`

### 3) Expanded Shell Command Set
- Core: `help`, `clear`/`cls`, `mem`, `echo`, `dmesg`, `version`, `about`, `banner`, `logo`
- Utility: `history`, `keys`, `alloc [size]`, `free`, `reboot`, `halt`
- UI/input: `prompt kali|simple`, `ui ...`, `status on|off`, `mouse status|on|off|pos <row> <col>`
- Apps:
  - `app list`
  - `app info <name>`
  - `app run <name>`
  - `app hostrun <name>`
  - `app install <git-url>`
  - `git install <git-url>`
- Python bridge:
  - `py ping`
  - `py exec <code>`
  - `py run <app-name>`

### 4) App + Host Workflow Improvements
- App manifest pipeline expanded for built-in + host-installed apps
- Host-assisted install/run flow documented and integrated into shell commands
- Better path for testing Python-based apps through serial bridge workflow

### 5) Tooling / Launcher Improvements
- Added IROS-branded wrappers:
  - `iros-vm.ps1`
  - `iros-emulator.ps1`
  - `iros-emulator.cmd`
- Improved QEMU run defaults and fallback behavior in scripts
- Updated README with setup, install commands, and troubleshooting flow

---

## Run (Windows, quick start)

```powershell
cd C:\Users\rohan\CLionProjects\IROS
.\build.ps1
.\iros-emulator.ps1 -Build -Display sdl
```

If needed:

```powershell
$env:QEMU_HOME = "C:\Program Files\qemu"
```

---

## Known Notes / Limits
- Python execution is host-bridged (via serial TCP), not in-kernel native Python.
- Git/app install is host-assisted (clone + manifest + rebuild), not network-native in kernel space.
- Full graphical desktop/window manager is still out of scope for this release (text-mode UI focus).

---

## Upgrade Impact
- Recommended for all users on `v1.x` who want stable interactive input and richer shell behavior.
- No external OS dependency introduced in kernel runtime path.
- Existing build flows (`build.ps1`, `run.ps1`, `Makefile`) remain available.
