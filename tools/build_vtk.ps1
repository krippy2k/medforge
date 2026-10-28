param(
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Release",
    [string]$SourceDir = "third_party/src/VTK-9.4.2",
    [string]$BuildDir = "",
    [string]$Prefix = ""
)

if (-not $BuildDir) {
    $BuildDir = "third_party/build/vtk-9.4.2-windows-msvc"
    if ($BuildType -eq "Debug") { $BuildDir += "-debug" }
}
if (-not $Prefix) {
    $Prefix = "third_party/dist/vtk-9.4.2-windows-msvc"
    if ($BuildType -eq "Debug") { $Prefix += "-debug" }
}

$ErrorActionPreference = "Stop"

# MSVC 19.40+ defines _SECURE_SCL as 0 and removed stdext::checked_array_iterator.
# diy2's fmt uses #ifdef _SECURE_SCL, which is true even when the macro is 0.
$fmtGuard = "#if defined(_SECURE_SCL) && _SECURE_SCL && _MSC_VER < 1940"
$fmtHeaders = @(
    (Join-Path $SourceDir "ThirdParty/diy2/vtkdiy2/include/vtkdiy2/fmt/format.h"),
    (Join-Path $SourceDir "ThirdParty/vtkm/vtkvtkm/vtk-m/vtkm/thirdparty/diy/vtkmdiy/include/vtkmdiy/thirdparty/fmt/format.h")
)
foreach ($fmtHeader in $fmtHeaders) {
    if (-not (Test-Path $fmtHeader)) { continue }
    $fmtText = [System.IO.File]::ReadAllText($fmtHeader)
    if ($fmtText.Contains($fmtGuard)) { continue }
    $updated = $fmtText.Replace("#ifdef _SECURE_SCL", $fmtGuard)
    if (-not $updated.Contains($fmtGuard)) {
        $updated = $updated.Replace("#if defined(_SECURE_SCL) && _SECURE_SCL", $fmtGuard)
    }
    if ($updated -eq $fmtText) { continue }
    [System.IO.File]::WriteAllText($fmtHeader, $updated)
}

# Windows PowerShell does not expand $variables after '=' in native arguments.
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
    -DCMAKE_PREFIX_PATH="C:/Qt/6.12.0/msvc2022_64" `
    -DBUILD_SHARED_LIBS=ON `
    -DVTK_BUILD_TESTING=OFF `
    -DVTK_BUILD_EXAMPLES=OFF `
    -DVTK_BUILD_ALL_MODULES=OFF `
    -DVTK_ENABLE_WRAPPING=OFF `
    -DVTK_WRAP_PYTHON=OFF `
    -DVTK_GROUP_ENABLE_Imaging=DONT_WANT `
    -DVTK_GROUP_ENABLE_MPI=DONT_WANT `
    -DVTK_GROUP_ENABLE_Qt=DONT_WANT `
    -DVTK_GROUP_ENABLE_Rendering=DONT_WANT `
    -DVTK_GROUP_ENABLE_StandAlone=DONT_WANT `
    -DVTK_GROUP_ENABLE_Views=DONT_WANT `
    -DVTK_GROUP_ENABLE_Web=DONT_WANT `
    -DVTK_QT_VERSION=6 `
    -DVTK_MODULE_ENABLE_VTK_GUISupportQt=YES `
    -DVTK_MODULE_ENABLE_VTK_InteractionStyle=YES `
    -DVTK_MODULE_ENABLE_VTK_RenderingAnnotation=YES `
    -DVTK_MODULE_ENABLE_VTK_FiltersSources=YES `
    -DVTK_MODULE_ENABLE_VTK_RenderingOpenGL2=YES
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build $BuildDir --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --install $BuildDir
