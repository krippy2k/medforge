param(
    [string]$SourceDir = "third_party/src/VTK-9.4.2",
    [string]$BuildDir = "third_party/build/vtk-9.4.2-windows-mingw",
    [string]$Prefix = "third_party/dist/vtk-9.4.2-windows-mingw"
)

$ErrorActionPreference = "Stop"
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;" + $env:PATH
cmake -S $SourceDir -B $BuildDir -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_INSTALL_PREFIX=$Prefix `
    -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe" `
    -DCMAKE_C_COMPILER="C:/Qt/Tools/mingw1310_64/bin/gcc.exe" `
    -DCMAKE_PREFIX_PATH="C:/Qt/6.12.0/mingw_64" `
    -DBUILD_SHARED_LIBS=OFF `
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
