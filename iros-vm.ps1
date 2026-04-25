param(
  [switch]$Build = $false,
  [ValidateSet("sdl","gtk","curses","none")]
  [string]$Display = "gtk",
  [switch]$Detach = $false,
  [string]$VmName = "IROS_VM"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $repo

if ($Build -or -not (Test-Path ".\build\iros.img")) {
  & .\build.ps1
}

$runArgs = @{
  Img = "build\iros.img"
  VmName = $VmName
  Display = $Display
}
if ($Detach.IsPresent) {
  $runArgs.Detach = $true
} else {
  $runArgs.Detach = $false
}

& .\run.ps1 @runArgs
