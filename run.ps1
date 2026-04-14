param(
  [string]$Img = "build\\iros.img",
  [ValidateSet("sdl","gtk","curses","none")]
  [string]$Display = "gtk",
  [switch]$Detach = $true,
  [switch]$NoReboot = $true,
  [switch]$NoShutdown = $true,
  [string]$DebugLog = "build\\qemu-debug.log",
  [switch]$UseGuiBinary = $false,
  [switch]$PyBridge = $false,
  [string]$PyBridgeHost = "127.0.0.1",
  [int]$PyBridgePort = 4444
)

$ErrorActionPreference = "Stop"

function Find-Qemu {
  $cmd = Get-Command qemu-system-i386 -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Source }

  $candidates = @()

  if ($env:QEMU_HOME) {
    $candidates += (Join-Path $env:QEMU_HOME "qemu-system-i386.exe")
    $candidates += (Join-Path $env:QEMU_HOME "qemu-system-i386w.exe")
  }

  $candidates += "C:\\Program Files\\qemu\\qemu-system-i386.exe"
  $candidates += "C:\\Program Files\\qemu\\qemu-system-i386w.exe"
  $candidates += "C:\\Program Files (x86)\\qemu\\qemu-system-i386.exe"
  $candidates += "C:\\Program Files (x86)\\qemu\\qemu-system-i386w.exe"

  foreach ($p in $candidates) {
    if (Test-Path $p) { return $p }
  }

  return $null
}

$qemuPath = Find-Qemu
if (-not $qemuPath) {
  Write-Host "qemu-system-i386 not found."
  Write-Host "Install QEMU and either add it to PATH, or set QEMU_HOME to its install folder."
  Write-Host "Example:"
  Write-Host "  `$env:QEMU_HOME = 'C:\\Program Files\\qemu'"
  exit 1
}

if (-not (Test-Path $Img)) {
  Write-Host "Image not found: $Img"
  Write-Host "Build it first with: .\\build.ps1"
  exit 1
}

# Use console binary by default (better error reporting). Opt-in to the GUI-subsystem binary.
$qemuDir = Split-Path -Parent $qemuPath
if ($UseGuiBinary) {
  $qemuW = Join-Path $qemuDir "qemu-system-i386w.exe"
  if (Test-Path $qemuW) { $qemuPath = $qemuW }
}

$args = @("-display", $Display, "-drive", "format=raw,file=$Img")
if ($NoReboot) { $args += "-no-reboot" }
if ($NoShutdown) { $args += "-no-shutdown" }

# Always collect a debug log to help diagnose boot loops/triple faults.
if ($DebugLog) {
  $args += @("-d", "int,guest_errors,cpu_reset", "-D", $DebugLog)
}

if ($PyBridge) {
  # Expose COM1 as a TCP server; the host connects with tools/iros-pybridge.py.
  $args += @("-serial", "tcp:$PyBridgeHost`:$PyBridgePort,server,nowait")
}

if ($Detach) {
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $DebugLog) | Out-Null
  $out = "build\\qemu-stdout.log"
  $err = "build\\qemu-stderr.log"
  $p = Start-Process -FilePath $qemuPath -ArgumentList $args -PassThru -RedirectStandardOutput $out -RedirectStandardError $err
  Write-Host "Started QEMU (PID $($p.Id)). Logs: $out, $err, $DebugLog"
  if ($PyBridge) {
    Write-Host "Python bridge: COM1 is listening on tcp:$PyBridgeHost`:$PyBridgePort (connect: python tools\\iros-pybridge.py)"
  }
} else {
  & $qemuPath @args
}
