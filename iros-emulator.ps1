param(
  [switch]$Build = $false,
  [string]$Image = "build\\iros.img",
  [ValidateSet("sdl","gtk","curses","none")]
  [string]$Display = "gtk",
  [switch]$Detach = $false,
  [string]$Name = "IROS Emulator"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $repo

if ($Build -or -not (Test-Path $Image)) {
  & .\build.ps1
}

& .\run.ps1 `
  -Img $Image `
  -VmName $Name `
  -Display $Display `
  -Detach:$Detach
