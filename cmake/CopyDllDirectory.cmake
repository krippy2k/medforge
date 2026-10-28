if(NOT SOURCE_DIR OR NOT DEST_DIR)
    message(FATAL_ERROR "CopyDllDirectory.cmake requires SOURCE_DIR and DEST_DIR")
endif()
file(GLOB _medforge_runtime_dlls "${SOURCE_DIR}/*.dll")
foreach(_medforge_runtime_dll IN LISTS _medforge_runtime_dlls)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${_medforge_runtime_dll}" "${DEST_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endforeach()
