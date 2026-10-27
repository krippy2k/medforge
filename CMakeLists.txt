cmake_minimum_required(VERSION 3.27)

project(MedForge
    VERSION 0.1.0
    DESCRIPTION "CT-to-CAD research workstation"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

option(MEDFORGE_BUILD_TESTS "Build the MedForge automated tests" ON)
option(MEDFORGE_BUILD_TOOLS "Build developer fixture tools" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(MedForgeWarnings)

if(NOT CMAKE_PREFIX_PATH)
    set(_medforge_qt_candidates)
    if(WIN32)
        list(APPEND _medforge_qt_candidates "C:/Qt/6.12.0/mingw_64")
    else()
        list(APPEND _medforge_qt_candidates
            "$ENV{HOME}/Qt/6.12.0/gcc_64"
            "/opt/Qt/6.12.0/gcc_64")
    endif()
    foreach(_medforge_qt_candidate IN LISTS _medforge_qt_candidates)
        if(EXISTS "${_medforge_qt_candidate}/lib/cmake/Qt6/Qt6Config.cmake")
            list(PREPEND CMAKE_PREFIX_PATH "${_medforge_qt_candidate}")
            break()
        endif()
    endforeach()
    unset(_medforge_qt_candidate)
    unset(_medforge_qt_candidates)
endif()

find_package(Qt6 6.12 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()
# qt_standard_project_setup() marks executables as Win32 GUI apps, which discards
# test and command-line output. Keep a console entry point for CTest and --help.
set(CMAKE_WIN32_EXECUTABLE OFF)

if(MEDFORGE_BUILD_TESTS)
    find_package(Qt6 6.12 REQUIRED COMPONENTS Test)
    enable_testing()
    include(MedForgeTests)
    if(Qt6_DIR)
        get_filename_component(MEDFORGE_QT_BIN "${Qt6_DIR}/../../../bin" ABSOLUTE)
    endif()
    if(WIN32 AND CMAKE_CXX_COMPILER)
        get_filename_component(MEDFORGE_MINGW_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    endif()
endif()

add_subdirectory(src/core)
add_subdirectory(src/imaging)
add_subdirectory(src/geometry)
add_subdirectory(src/cad)
add_subdirectory(src/rendering)
add_subdirectory(src/ui)
add_subdirectory(apps/desktop)
if(MEDFORGE_BUILD_TOOLS)
    add_subdirectory(apps/fixtures)
endif()

if(MEDFORGE_BUILD_TESTS)
    add_subdirectory(tests)
endif()
