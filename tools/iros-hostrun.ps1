param(
  [Parameter(Mandatory=$true)][string]$Name,
  [string]$Entry = ""
)

$ErrorActionPreference = "Stop"

function Require-Cmd($name) {
  $cmd = Get-Command $name -ErrorAction SilentlyContinue
  if (-not $cmd) { throw "Missing tool '$name' on PATH." }
}

Require-Cmd python

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent $root

$repoDir = Join-Path $root ("apps-src\\" + $Name)
if (-not (Test-Path $repoDir)) {
  throw "App repo not found: $repoDir. Install it first with tools\\iros-install.ps1."
}

if (-not $Entry) {
  $manifest = Join-Path $root ("apps\\" + $Name + ".app")
  if (Test-Path $manifest) {
    $lines = Get-Content $manifest -ErrorAction SilentlyContinue
    foreach ($l in $lines) {
      if ($l -match "^\s*entry\s*=\s*(.+)\s*$") { $Entry = $Matches[1].Trim(); break }
    }
  }
}

if (-not $Entry) {
  $candidates = @("main.py", "$Name.py", "app.py", "run.py", "__main__.py")
  foreach ($c in $candidates) {
    $p = Join-Path $repoDir $c
    if (Test-Path $p) { $Entry = $c; break }
  }
}

if (-not $Entry) {
  throw "No Python entrypoint found. Specify one: tools\\iros-hostrun.ps1 $Name <entry.py>"
}

$req = Join-Path $repoDir "requirements.txt"
if (Test-Path $req) {
  Write-Host "Installing Python deps from requirements.txt..."
  python -m pip install -r $req
}

Write-Host "Running: python $Entry"
python (Join-Path $repoDir $Entry)

