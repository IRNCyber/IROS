param(
  [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"

function Require-Cmd($name) {
  $cmd = Get-Command $name -ErrorAction SilentlyContinue
  if (-not $cmd) { throw "Missing required tool '$name' on PATH." }
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$useGnu = $false
$cc = $null
$ld = $null
$objcopy = $null
$ccTarget = @()

if (Get-Command i686-elf-gcc -ErrorAction SilentlyContinue) {
  $useGnu = $true
  Require-Cmd i686-elf-gcc
  Require-Cmd i686-elf-ld
  Require-Cmd i686-elf-objcopy
  $cc = "i686-elf-gcc"
  $ld = "i686-elf-ld"
  $objcopy = "i686-elf-objcopy"
} else {
  # LLVM toolchain (recommended on Windows): clang + ld.lld + llvm-objcopy
  Require-Cmd clang
  Require-Cmd ld.lld
  Require-Cmd llvm-objcopy
  $cc = "clang"
  $ld = "ld.lld"
  $objcopy = "llvm-objcopy"
  $ccTarget = @("--target=i686-elf")
}

$cflags = @(
  "-m32",
  "-ffreestanding",
  "-fno-stack-protector",
  "-fno-builtin",
  "-fno-asynchronous-unwind-tables",
  "-fno-unwind-tables",
  "-fno-pic",
  "-I", "include",
  "-O2",
  "-Wall",
  "-Wextra"
)

& $cc @ccTarget @cflags -c kernel\entry.S -o "$BuildDir\entry.o"
& $cc @ccTarget @cflags -c kernel\kernel.c -o "$BuildDir\kernel.o"
& $cc @ccTarget @cflags -c kernel\vga.c -o "$BuildDir\vga.o"
& $cc @ccTarget @cflags -c kernel\memory.c -o "$BuildDir\memory.o"
& $cc @ccTarget @cflags -c lib\string.c -o "$BuildDir\string.o"

& $ld -m elf_i386 -T linker.ld -o "$BuildDir\kernel.elf" `
  "$BuildDir\entry.o" "$BuildDir\kernel.o" "$BuildDir\vga.o" "$BuildDir\memory.o" "$BuildDir\string.o"

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
