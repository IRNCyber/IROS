param(
  [Parameter(Mandatory=$true)][string]$Repo,
  [string]$Name = ""
)

$ErrorActionPreference = "Stop"

function Require-Cmd($name) {
  $cmd = Get-Command $name -ErrorAction SilentlyContinue
  if (-not $cmd) { throw "Missing tool '$name' on PATH." }
}

Require-Cmd git
Require-Cmd python

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent $root

$appsDir = Join-Path $root "apps-installed"
$srcDir = Join-Path $root "apps-src"
New-Item -ItemType Directory -Force -Path $appsDir | Out-Null
New-Item -ItemType Directory -Force -Path $srcDir | Out-Null

if (-not $Name) {
  $Name = [IO.Path]::GetFileNameWithoutExtension($Repo.TrimEnd('/').Split('/')[-1])
  if (-not $Name) { $Name = "app" }
}

$dstRepo = Join-Path $srcDir $Name
if (Test-Path $dstRepo) {
  Write-Host "Updating $dstRepo"
  git -C $dstRepo pull --ff-only
} else {
  Write-Host "Cloning $Repo -> $dstRepo"
  git clone $Repo $dstRepo
}

$manifest = Join-Path $dstRepo "iros.app"
$outApp = Join-Path $appsDir ("$Name.app")

if (Test-Path $manifest) {
  Copy-Item -Force $manifest $outApp
} else {
  $desc = "Git app: $Repo"
  $type = "text"
  $entry = ""
  $text = ""

  $hasPython = (Test-Path (Join-Path $dstRepo "requirements.txt")) -or (Test-Path (Join-Path $dstRepo "pyproject.toml")) -or (Test-Path (Join-Path $dstRepo "setup.py"))
  if (-not $hasPython) {
    $py = Get-ChildItem -Path $dstRepo -Recurse -Filter *.py -File -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($py) { $hasPython = $true }
  }
  if ($hasPython) {
    $type = "python"
    $candidates = @("main.py", "$Name.py", "app.py", "run.py", "__main__.py")
    foreach ($c in $candidates) {
      if (Test-Path (Join-Path $dstRepo $c)) { $entry = $c; break }
    }
  }

  $readme = Join-Path $dstRepo "README.md"
  if (Test-Path $readme) {
    $text = (Get-Content $readme -TotalCount 30) -join "`n"
  } else {
    $text = "Installed from: $Repo"
  }

  @(
    "name=$Name"
    "desc=$desc"
    "type=$type"
    "entry=$entry"
    "---"
    $text
  ) | Set-Content -Encoding UTF8 $outApp
}

Write-Host "Generating kernel app table..."
python (Join-Path $root "tools\\gen_apps.py") --include-installed

Write-Host "Installed app '$Name' into $outApp"
Write-Host "Rebuild with: .\\build.ps1"
