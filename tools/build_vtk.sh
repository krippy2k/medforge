#!/usr/bin/env bash
set -euo pipefail

source_dir="${1:-third_party/src/VTK-9.4.2}"
build_dir="${2:-${HOME}/vtk/build-9.4.2}"
prefix="${3:-${HOME}/vtk/9.4.2}"
qt_prefix="${CMAKE_PREFIX_PATH:-${HOME}/Qt/6.12.0/gcc_64}"

cmake -S "${source_dir}" -B "${build_dir}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${prefix}" \
    -DCMAKE_PREFIX_PATH="${qt_prefix}" \
    -DCMAKE_INSTALL_RPATH="\$ORIGIN;${qt_prefix}/lib" \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
    -DBUILD_SHARED_LIBS=ON \
    -DVTK_BUILD_TESTING=OFF \
    -DVTK_BUILD_EXAMPLES=OFF \
    -DVTK_BUILD_ALL_MODULES=OFF \
    -DVTK_ENABLE_WRAPPING=OFF \
    -DVTK_WRAP_PYTHON=OFF \
    -DVTK_GROUP_ENABLE_Imaging=DONT_WANT \
    -DVTK_GROUP_ENABLE_MPI=DONT_WANT \
    -DVTK_GROUP_ENABLE_Qt=DONT_WANT \
    -DVTK_GROUP_ENABLE_Rendering=DONT_WANT \
    -DVTK_GROUP_ENABLE_StandAlone=DONT_WANT \
    -DVTK_GROUP_ENABLE_Views=DONT_WANT \
    -DVTK_GROUP_ENABLE_Web=DONT_WANT \
    -DVTK_QT_VERSION=6 \
    -DVTK_MODULE_ENABLE_VTK_GUISupportQt=YES \
    -DVTK_MODULE_ENABLE_VTK_InteractionStyle=YES \
    -DVTK_MODULE_ENABLE_VTK_RenderingAnnotation=YES \
    -DVTK_MODULE_ENABLE_VTK_FiltersSources=YES \
    -DVTK_MODULE_ENABLE_VTK_RenderingOpenGL2=YES
cmake --build "${build_dir}" --parallel
cmake --install "${build_dir}"
