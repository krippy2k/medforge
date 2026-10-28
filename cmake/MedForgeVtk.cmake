# The installed release package has only Release import libraries. A Debug
# build would otherwise link those and then fail to load Qt6Core.dll,
# because the debug app deploys Qt6Cored.dll. Point Debug at the debug kit.
function(medforge_vtk_use_debug_libraries)
    if(NOT VTK_FOUND OR NOT VTK_DIR)
        return()
    endif()
    set(_medforge_vtk_debug_root "${CMAKE_SOURCE_DIR}/third_party/dist/vtk-9.4.2-windows-msvc-debug")
    if(NOT EXISTS "${_medforge_vtk_debug_root}/bin" OR
       NOT EXISTS "${VTK_DIR}/VTK-targets-release.cmake")
        return()
    endif()
    file(STRINGS "${VTK_DIR}/VTK-targets-release.cmake" _medforge_vtk_target_lines
        REGEX "^set_property\\(TARGET VTK::")
    foreach(_medforge_vtk_target_line IN LISTS _medforge_vtk_target_lines)
        if(NOT _medforge_vtk_target_line MATCHES "TARGET (VTK::[^ ]+)")
            continue()
        endif()
        set(_medforge_vtk_target "${CMAKE_MATCH_1}")
        if(NOT TARGET ${_medforge_vtk_target})
            continue()
        endif()
        get_target_property(_medforge_vtk_rel_dll ${_medforge_vtk_target} IMPORTED_LOCATION_RELEASE)
        get_target_property(_medforge_vtk_rel_lib ${_medforge_vtk_target} IMPORTED_IMPLIB_RELEASE)
        if(NOT _medforge_vtk_rel_dll OR NOT _medforge_vtk_rel_lib)
            continue()
        endif()
        get_filename_component(_medforge_vtk_dll_name "${_medforge_vtk_rel_dll}" NAME)
        get_filename_component(_medforge_vtk_lib_name "${_medforge_vtk_rel_lib}" NAME)
        string(REGEX REPLACE "\\.dll$" "d.dll" _medforge_vtk_dll_name "${_medforge_vtk_dll_name}")
        string(REGEX REPLACE "\\.lib$" "d.lib" _medforge_vtk_lib_name "${_medforge_vtk_lib_name}")
        set(_medforge_vtk_dbg_dll "${_medforge_vtk_debug_root}/bin/${_medforge_vtk_dll_name}")
        set(_medforge_vtk_dbg_lib "${_medforge_vtk_debug_root}/lib/${_medforge_vtk_lib_name}")
        if(EXISTS "${_medforge_vtk_dbg_dll}" AND EXISTS "${_medforge_vtk_dbg_lib}")
            set_property(TARGET ${_medforge_vtk_target} APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(${_medforge_vtk_target} PROPERTIES
                IMPORTED_LOCATION_DEBUG "${_medforge_vtk_dbg_dll}"
                IMPORTED_IMPLIB_DEBUG "${_medforge_vtk_dbg_lib}"
                MAP_IMPORTED_CONFIG_DEBUG "DEBUG")
            get_target_property(_medforge_vtk_deps ${_medforge_vtk_target}
                IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE)
            if(_medforge_vtk_deps)
                set_target_properties(${_medforge_vtk_target} PROPERTIES
                    IMPORTED_LINK_DEPENDENT_LIBRARIES_DEBUG "${_medforge_vtk_deps}")
            endif()
        endif()
    endforeach()
endfunction()
