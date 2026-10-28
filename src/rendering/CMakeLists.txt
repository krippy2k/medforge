add_library(medforge_rendering STATIC
    src/CameraPose.cpp
    src/Isosurface.cpp
    src/Module.cpp
    src/TransferFunction.cpp
)
add_library(medforge::rendering ALIAS medforge_rendering)
target_include_directories(medforge_rendering PUBLIC include)
target_link_libraries(medforge_rendering PUBLIC medforge::core medforge::geometry)
target_compile_features(medforge_rendering PUBLIC cxx_std_20)
set_target_properties(medforge_rendering PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
medforge_set_core_warnings(medforge_rendering)

set(_medforge_vtk_candidates)
if(WIN32)
    if(MSVC)
        list(APPEND _medforge_vtk_candidates
            "${CMAKE_SOURCE_DIR}/third_party/dist/vtk-9.4.2-windows-msvc/lib/cmake/vtk-9.4")
    endif()
else()
    list(APPEND _medforge_vtk_candidates
        "${CMAKE_SOURCE_DIR}/third_party/dist/vtk-9.4.2-linux/lib/cmake/vtk-9.4")
    if(DEFINED ENV{HOME})
        list(APPEND _medforge_vtk_candidates "$ENV{HOME}/vtk/9.4.2/lib/cmake/vtk-9.4")
    endif()
endif()
if(NOT VTK_DIR)
    foreach(_medforge_vtk_candidate IN LISTS _medforge_vtk_candidates)
        if(EXISTS "${_medforge_vtk_candidate}/vtk-config.cmake")
            set(VTK_DIR "${_medforge_vtk_candidate}" CACHE PATH "VTK cmake package" FORCE)
            break()
        endif()
    endforeach()
endif()
find_package(VTK 9.4 QUIET CONFIG COMPONENTS
    CommonCore
    CommonDataModel
    FiltersCore
)
if(VTK_FOUND)
    include(MedForgeVtk)
    medforge_vtk_use_debug_libraries()
    target_compile_definitions(medforge_rendering PUBLIC MEDFORGE_HAS_VTK)
    target_link_libraries(medforge_rendering PUBLIC VTK::CommonCore VTK::CommonDataModel VTK::FiltersCore)
    vtk_module_autoinit(TARGETS medforge_rendering MODULES VTK::FiltersCore)
endif()
