param(
    [string]$BuildDir = "build/windows-msvc",
    [string]$OutputDir = "build/medforge-0.1.0-windows-unsigned"
)

$ErrorActionPreference = "Stop"
$bin = Join-Path $BuildDir "bin/Debug"
if (-not (Test-Path (Join-Path $bin "medforge.exe"))) {
    $bin = Join-Path $BuildDir "bin"
}
$exe = Join-Path $bin "medforge.exe"
if (-not (Test-Path $exe)) {
    throw "Executable not found under $BuildDir"
}

if (Test-Path $OutputDir) {
    Remove-Item -Recurse -Force $OutputDir
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

Get-ChildItem $bin -File | Where-Object {
    $_.Extension -eq ".dll" -or $_.Name -in @("medforge.exe", "medforge_fixtures.exe")
} | ForEach-Object {
    Copy-Item $_.FullName (Join-Path $OutputDir $_.Name)
}
foreach ($child in @("platforms", "imageformats", "iconengines", "styles")) {
    $source = Join-Path $bin $child
    if (Test-Path $source) {
        Copy-Item $source (Join-Path $OutputDir $child) -Recurse
    }
}

$notice = @"
UNSIGNED RESEARCH BUILD
This package is not signed or notarized.
Research/demo only - not for clinical use.
Do not use for diagnosis, treatment, or manufacture.
"@
Set-Content -Path (Join-Path $OutputDir "UNSIGNED.txt") -Value $notice -Encoding utf8
Copy-Item (Join-Path (Split-Path $PSScriptRoot -Parent) "docs/RELEASE.txt") (Join-Path $OutputDir "RELEASE.txt")

$copied = Join-Path $OutputDir "medforge.exe"
if (-not (Test-Path $copied)) {
    throw "Package is missing medforge.exe"
}
$label = Get-Content (Join-Path $OutputDir "UNSIGNED.txt") -Raw
if ($label -notmatch "UNSIGNED" -or $label -notmatch "not for clinical use") {
    throw "Package notice is missing the unsigned research label"
}
Write-Output "Unsigned package: $OutputDir"
