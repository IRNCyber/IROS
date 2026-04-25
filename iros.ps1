param(
  [switch]$Build = $false,
  [ValidateSet("sdl","gtk","curses","none")]
  [string]$Display = "gtk",
  [switch]$Detach = $false,
  [string]$Name = "IROS"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $repo

& .\iros-vm.ps1 -Build:$Build -Display $Display -Detach:$Detach -VmName $Name
