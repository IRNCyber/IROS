param(
  [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"

function Find-Tool($name, [string[]]$candidates) {
  $cmd = Get-Command $name -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Source }

  foreach ($p in $candidates) {
    if ($p -and (Test-Path $p)) { return $p }
  }

  return $null
}

function Require-Tool($name, [string[]]$candidates = @()) {
  $path = Find-Tool $name $candidates
  if (-not $path) { throw "Missing required tool '$name'. Install it or add it to PATH." }
  return $path
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$python = Get-Command python -ErrorAction SilentlyContinue
if ($python) {
  try {
    & $python.Source tools\gen_apps.py | Out-Host
  } catch {
    Write-Host "Warning: app codegen failed; continuing with existing kernel\\apps_gen.c"
  }
}

$useGnu = $false
$cc = $null
$ld = $null
$objcopy = $null
$ccTarget = @()

if (Get-Command i686-elf-gcc -ErrorAction SilentlyContinue) {
  $useGnu = $true
  $cc = Require-Tool "i686-elf-gcc"
  $ld = Require-Tool "i686-elf-ld"
  $objcopy = Require-Tool "i686-elf-objcopy"
} else {
  # LLVM toolchain (recommended on Windows): clang + ld.lld + llvm-objcopy
  $llvmBin = @(
    "C:\Program Files\LLVM\bin",
    "C:\Program Files (x86)\LLVM\bin"
  )

  $cc = Require-Tool "clang" @(
    (Join-Path $llvmBin[0] "clang.exe"),
    (Join-Path $llvmBin[1] "clang.exe")
  )

  $ld = Require-Tool "ld.lld" @(
    (Join-Path $llvmBin[0] "ld.lld.exe"),
    (Join-Path $llvmBin[1] "ld.lld.exe")
  )

  $objcopy = Require-Tool "llvm-objcopy" @(
    (Join-Path $llvmBin[0] "llvm-objcopy.exe"),
    (Join-Path $llvmBin[1] "llvm-objcopy.exe")
  )

  $ccTarget = @("--target=i686-elf")
}

$cflags = @(
  "-m32",
  "-ffreestanding",
  "-fno-stack-protector",
  "-fno-builtin",
  "-fno-asynchronous-unwind-tables",
  "-fno-unwind-tables",
  "-mno-sse",
  "-mno-sse2",
  "-mno-mmx",
  "-mno-80387",
  "-msoft-float",
  "-fno-pic",
  "-I", "include",
  "-O2",
  "-Wall",
  "-Wextra"
)

& $cc @ccTarget @cflags -c kernel\entry.S -o "$BuildDir\entry.o"
& $cc @ccTarget @cflags -c kernel\isr_stub.S -o "$BuildDir\isr_stub.o"
& $cc @ccTarget @cflags -c kernel\kernel.c -o "$BuildDir\kernel.o"
& $cc @ccTarget @cflags -c kernel\vga.c -o "$BuildDir\vga.o"
& $cc @ccTarget @cflags -c kernel\log.c -o "$BuildDir\log.o"
& $cc @ccTarget @cflags -c kernel\memory.c -o "$BuildDir\memory.o"
& $cc @ccTarget @cflags -c kernel\idt.c -o "$BuildDir\idt.o"
& $cc @ccTarget @cflags -c kernel\pic.c -o "$BuildDir\pic.o"
& $cc @ccTarget @cflags -c kernel\isr.c -o "$BuildDir\isr.o"
& $cc @ccTarget @cflags -c kernel\keyboard.c -o "$BuildDir\keyboard.o"
& $cc @ccTarget @cflags -c kernel\shell.c -o "$BuildDir\shell.o"
& $cc @ccTarget @cflags -c kernel\status.c -o "$BuildDir\status.o"
& $cc @ccTarget @cflags -c kernel\serial.c -o "$BuildDir\serial.o"
& $cc @ccTarget @cflags -c kernel\apps.c -o "$BuildDir\apps.o"
& $cc @ccTarget @cflags -c kernel\apps_gen.c -o "$BuildDir\apps_gen.o"
& $cc @ccTarget @cflags -c lib\string.c -o "$BuildDir\string.o"

& $ld -m elf_i386 -T linker.ld -o "$BuildDir\kernel.elf" `
  "$BuildDir\entry.o" "$BuildDir\isr_stub.o" "$BuildDir\kernel.o" "$BuildDir\vga.o" "$BuildDir\log.o" `
  "$BuildDir\memory.o" "$BuildDir\idt.o" "$BuildDir\pic.o" "$BuildDir\isr.o" "$BuildDir\keyboard.o" `
  "$BuildDir\shell.o" "$BuildDir\status.o" "$BuildDir\serial.o" "$BuildDir\apps.o" "$BuildDir\apps_gen.o" "$BuildDir\string.o"

& $objcopy -O binary "$BuildDir\kernel.elf" "$BuildDir\kernel.bin"

$kernelBytes = [System.IO.File]::ReadAllBytes("$BuildDir\kernel.bin")
$sectors = [int][Math]::Ceiling($kernelBytes.Length / 512.0)
if ($sectors -gt 127) { throw "Kernel too large ($sectors sectors); boot sector loader supports up to 127." }

& $cc @ccTarget -c "-DKERNEL_SECTORS=$sectors" boot\boot.S -o "$BuildDir\boot.o"
& $ld -m elf_i386 -T boot\boot.ld -o "$BuildDir\boot.elf" "$BuildDir\boot.o"
& $objcopy -O binary "$BuildDir\boot.elf" "$BuildDir\boot.bin"

# Create 1.44MB floppy image and write boot + kernel
$imgPath = Join-Path $BuildDir "iros.img"
$imgSize = 1474560

$fs = [System.IO.File]::Open($imgPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::ReadWrite)
try {
  $fs.SetLength($imgSize)

  $boot = [System.IO.File]::ReadAllBytes("$BuildDir\boot.bin")
  if ($boot.Length -ne 512) { throw "boot.bin must be exactly 512 bytes (got $($boot.Length))." }
  $fs.Position = 0
  $fs.Write($boot, 0, $boot.Length)

  # Pad kernel to whole sectors
  $paddedLen = $sectors * 512
  $kernelPadded = New-Object byte[] $paddedLen
  [Array]::Copy($kernelBytes, $kernelPadded, $kernelBytes.Length)

  $fs.Position = 512
  $fs.Write($kernelPadded, 0, $kernelPadded.Length)
}
finally {
  $fs.Dispose()
}

Write-Host "Built $imgPath (kernel: $($kernelBytes.Length) bytes, $sectors sectors)"
