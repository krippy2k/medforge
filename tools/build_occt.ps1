param(
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Debug",
    [string]$SourceDir = "third_party/src/OCCT-7_9_3",
    [string]$BuildDir = "",
    [string]$Prefix = ""
)

if (-not $BuildDir) {
    $BuildDir = "third_party/build/occt-7.9.3-windows-msvc"
    if ($BuildType -eq "Debug") { $BuildDir += "-debug" }
}
if (-not $Prefix) {
    $Prefix = "third_party/dist/occt-7.9.3-windows-msvc"
    if ($BuildType -eq "Debug") { $Prefix += "-debug" }
}

$ErrorActionPreference = "Stop"
if (-not (Test-Path $SourceDir)) {
    throw "Open CASCADE source was not found at $SourceDir"
}

$InstallPrefix = [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $Prefix)).Replace('\', '/')

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
$install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $install) {
    throw "Visual Studio C++ tools were not found."
}
$vcvars = Join-Path $install "VC\Auxiliary\Build\vcvars64.bat"
cmd /c "`"$vcvars`" >nul && set" | ForEach-Object {
    if ($_ -match "^(.*?)=(.*)$") {
        Set-Item -Path "Env:$($Matches[1])" -Value $Matches[2]
    }
}

cmake -S $SourceDir -B $BuildDir -G Ninja `
    "-DCMAKE_BUILD_TYPE=$BuildType" `
    "-DCMAKE_INSTALL_PREFIX=$InstallPrefix" `
    -DBUILD_LIBRARY_TYPE=Shared `
    -DBUILD_MODULE_Draw=OFF `
    -DBUILD_MODULE_Visualization=OFF `
    -DBUILD_MODULE_ApplicationFramework=OFF `
    -DBUILD_MODULE_DETools=OFF `
    -DBUILD_MODULE_FoundationClasses=ON `
    -DBUILD_MODULE_ModelingData=ON `
    -DBUILD_MODULE_ModelingAlgorithms=ON `
    -DBUILD_MODULE_DataExchange=ON `
    -DUSE_FREETYPE=OFF `
    -DUSE_FREEIMAGE=OFF `
    -DUSE_FFMPEG=OFF `
    -DUSE_OPENVR=OFF `
    -DUSE_RAPIDJSON=OFF `
    -DUSE_DRACO=OFF `
    -DUSE_TBB=OFF `
    -DUSE_VTK=OFF `
    -DUSE_OPENGL=OFF `
    -DUSE_GLES2=OFF `
    -DUSE_D3D=OFF `
    -DUSE_TK=OFF `
    -DBUILD_DOC_Overview=OFF `
    -DBUILD_USE_PCH=OFF `
    -DBUILD_YACCLEX=OFF `
    -DBUILD_SAMPLES_QT=OFF `
    -DINSTALL_TEST_CASES=OFF
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build $BuildDir --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --install $BuildDir
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Open CASCADE installed to $InstallPrefix"
