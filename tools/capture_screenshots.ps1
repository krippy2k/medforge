param(
    [string]$BuildDir = "build/windows-msvc",
    [string]$OutputDir = "docs/ui/evidence/M0"
)

$ErrorActionPreference = "Stop"
$exe = Join-Path $BuildDir "bin/Debug/medforge.exe"
if (-not (Test-Path $exe)) {
    $exe = Join-Path $BuildDir "bin/medforge.exe"
}
if (-not (Test-Path $exe)) {
    throw "Executable not found under $BuildDir"
}

$env:PATH = "C:\Qt\6.12.0\msvc2022_64\bin;" + $env:PATH
$env:QT_QPA_PLATFORM = "offscreen"
$env:QT_QPA_FONTDIR = "C:\Windows\Fonts"
$env:QT_ASSUME_STDERR_HAS_CONSOLE = "1"
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

foreach ($scale in @("1", "1.5")) {
    $env:QT_SCALE_FACTOR = $scale
    & $exe --capture $OutputDir
    if ($LASTEXITCODE -ne 0) {
        throw "Capture failed at QT_SCALE_FACTOR=$scale"
    }
}

Write-Host "Screenshots written to $OutputDir"
Write-Host "Inspect the PNGs and manifests before treating visual QA as passed."
