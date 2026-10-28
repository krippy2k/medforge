if(NOT SOURCE_DIR OR NOT DEST_DIR)
    message(FATAL_ERROR "CopyDllDirectory.cmake requires SOURCE_DIR and DEST_DIR")
endif()
if(NOT EXISTS "${SOURCE_DIR}")
    message(FATAL_ERROR "Runtime DLL directory does not exist: ${SOURCE_DIR}")
endif()
if(SOURCE_DIR MATCHES "msvc-debug")
    file(GLOB _medforge_stale_vtk_dlls "${DEST_DIR}/vtk*-9.4.dll")
    foreach(_medforge_stale_vtk_dll IN LISTS _medforge_stale_vtk_dlls)
        file(REMOVE "${_medforge_stale_vtk_dll}")
    endforeach()
endif()
file(GLOB _medforge_runtime_dlls "${SOURCE_DIR}/*.dll")
foreach(_medforge_runtime_dll IN LISTS _medforge_runtime_dlls)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${_medforge_runtime_dll}" "${DEST_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endforeach()
