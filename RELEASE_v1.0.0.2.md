## IROS v1.0.0.2 - Shell UX + Logging + Python Bridge

### Overview
v1.0.0.2 improves day-to-day usability: a better shell (history scrolling), a real kernel log buffer (`dmesg`), optional serial log mirroring, and an effective “run Python from IROS” workflow via a QEMU serial bridge.

---

## What’s Included

### Shell UX
- Command history scrolling via Up/Down arrows
- New commands:
  - `dmesg`
  - `log serial on|off`
  - `py ping|exec|run`

### Logging
- Ring-buffered kernel logs (in-kernel)
- Optional mirroring to COM1 for debugging on the host

### Python Bridge (Host-Assisted, Interactive)
- Run QEMU with `-PyBridge` to expose COM1 as TCP
- Run `tools/iros-pybridge.py` on the host to execute Python and stream output back into IROS

### CI
- GitHub Actions workflow builds and uploads `build/iros.img`

---

## How to Use Python Bridge

1) Start IROS with the bridge enabled:
```powershell
.\run.ps1 -PyBridge
```

2) In another terminal, connect the host bridge:
```powershell
python tools\iros-pybridge.py
```

3) In IROS:
```text
py ping
py exec print("hello from python")
py run tactages
```

