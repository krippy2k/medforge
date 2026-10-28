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
    if(MEDFORGE_VTK_BIN)
        add_custom_command(TARGET medforge POST_BUILD
            COMMAND "${CMAKE_COMMAND}"
                "-DSOURCE_DIR=${MEDFORGE_VTK_BIN}"
                "-DDEST_DIR=$<TARGET_FILE_DIR:medforge>"
                -P "${CMAKE_SOURCE_DIR}/cmake/CopyDllDirectory.cmake"
            COMMENT "Copying the VTK runtime next to medforge"
            VERBATIM)
    endif()
endif()
