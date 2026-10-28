add_executable(medforge main.cpp)
target_link_libraries(medforge PRIVATE medforge::ui)
medforge_set_warnings(medforge)

if(WIN32)
    find_program(MEDFORGE_WINDEPLOYQT windeployqt
        HINTS "${Qt6_DIR}/../../../bin"
        NO_DEFAULT_PATH
        REQUIRED)
    add_custom_command(TARGET medforge POST_BUILD
        COMMAND "${MEDFORGE_WINDEPLOYQT}"
            "$<IF:$<CONFIG:Debug>,--debug,--release>"
            --no-translations
            --no-compiler-runtime
            "$<TARGET_FILE:medforge>"
        COMMENT "Deploying the Qt runtime next to medforge"
        VERBATIM)
    add_custom_command(TARGET medforge POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:medforge>/platforms"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${Qt6_DIR}/../../../plugins/platforms/$<IF:$<CONFIG:Debug>,qoffscreend.dll,qoffscreen.dll>"
            "$<TARGET_FILE_DIR:medforge>/platforms/"
        COMMENT "Deploying the Qt offscreen plugin for tests"
        VERBATIM)
    set(_medforge_vtk_bin_release "${CMAKE_SOURCE_DIR}/third_party/dist/vtk-9.4.2-windows-msvc/bin")
    set(_medforge_vtk_bin_debug "${CMAKE_SOURCE_DIR}/third_party/dist/vtk-9.4.2-windows-msvc-debug/bin")
    if(EXISTS "${_medforge_vtk_bin_release}" OR EXISTS "${_medforge_vtk_bin_debug}")
        add_custom_command(TARGET medforge POST_BUILD
            COMMAND "${CMAKE_COMMAND}"
                "-DSOURCE_DIR=$<IF:$<CONFIG:Debug>,${_medforge_vtk_bin_debug},${_medforge_vtk_bin_release}>"
                "-DDEST_DIR=$<TARGET_FILE_DIR:medforge>"
                -P "${CMAKE_SOURCE_DIR}/cmake/CopyDllDirectory.cmake"
            COMMENT "Copying the VTK runtime next to medforge"
            VERBATIM)
    endif()
endif()
