add_library(medforge_ui STATIC
    include/medforge/ui/FoundationWidgets.h
    include/medforge/ui/FoundationWindow.h
    include/medforge/ui/GalleryWindow.h
    include/medforge/ui/NumericEditor.h
    include/medforge/ui/SegmentedControl.h
    include/medforge/ui/SampleViewport.h
    include/medforge/ui/WorkspaceActions.h
    include/medforge/ui/WorkspaceWindow.h
    src/ConfirmDialog.cpp
    src/FoundationWidgets.cpp
    src/FoundationWindow.cpp
    src/GalleryWindow.cpp
    src/Icons.cpp
    src/NumericEditor.cpp
    src/ScreenshotCapture.cpp
    src/SegmentedControl.cpp
    src/GuiThread.cpp
    src/LayoutPreferences.cpp
    src/SampleViewport.cpp
    src/Style.cpp
    src/WorkspaceActions.cpp
    src/WorkspaceReference.cpp
    src/WorkspaceWindow.cpp
)
add_library(medforge::ui ALIAS medforge_ui)

target_include_directories(medforge_ui PUBLIC include)
target_link_libraries(medforge_ui PUBLIC medforge::core medforge::rendering Qt6::Widgets)
target_compile_features(medforge_ui PUBLIC cxx_std_20)
medforge_set_warnings(medforge_ui)

set(_medforge_vtk_candidates)
if(WIN32)
    if(MSVC)
        list(APPEND _medforge_vtk_candidates
            "${CMAKE_SOURCE_DIR}/third_party/dist/vtk-9.4.2-windows-msvc/lib/cmake/vtk-9.4")
        if(VTK_DIR MATCHES "windows-mingw")
            unset(VTK_DIR CACHE)
        endif()
    else()
        list(APPEND _medforge_vtk_candidates
            "${CMAKE_SOURCE_DIR}/third_party/dist/vtk-9.4.2-windows-mingw/lib/cmake/vtk-9.4")
    endif()
    if(VTK_DIR MATCHES "linux")
        unset(VTK_DIR CACHE)
    endif()
else()
    list(APPEND _medforge_vtk_candidates
        "${CMAKE_SOURCE_DIR}/third_party/dist/vtk-9.4.2-linux/lib/cmake/vtk-9.4")
    if(DEFINED ENV{HOME})
        list(APPEND _medforge_vtk_candidates "$ENV{HOME}/vtk/9.4.2/lib/cmake/vtk-9.4")
    endif()
    if(VTK_DIR MATCHES "windows-mingw")
        unset(VTK_DIR CACHE)
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
    FiltersSources
    RenderingCore
    RenderingOpenGL2
    RenderingAnnotation
    InteractionStyle
    GUISupportQt
)
if(VTK_FOUND)
    # The installed release package has only Release import libraries. A Debug
    # build would otherwise link those and then fail to load Qt6Core.dll,
    # because the debug app deploys Qt6Cored.dll. Point Debug at the debug kit.
    set(_medforge_vtk_debug_root "${CMAKE_SOURCE_DIR}/third_party/dist/vtk-9.4.2-windows-msvc-debug")
    if(EXISTS "${_medforge_vtk_debug_root}/bin" AND EXISTS "${VTK_DIR}/VTK-targets-release.cmake")
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
    endif()
    target_compile_definitions(medforge_ui PRIVATE MEDFORGE_HAS_VTK)
    # Ubuntu also ships Qt 6.10. Link the kit's OpenGL libraries, and build VTK
    # with that kit on its install RPATH (tools/build_vtk.sh). Otherwise
    # libvtkGUISupportQt loads the distro OpenGL library and the sample never draws.
    if(UNIX AND NOT APPLE)
        get_filename_component(_medforge_qt_cmake "${Qt6_DIR}" DIRECTORY)
        foreach(_medforge_qt_pkg Qt6OpenGL Qt6OpenGLWidgets)
            if(DEFINED ${_medforge_qt_pkg}_DIR AND NOT ${_medforge_qt_pkg}_DIR MATCHES "6\\.12\\.0")
                unset(${_medforge_qt_pkg}_DIR CACHE)
            endif()
        endforeach()
        find_package(Qt6OpenGL REQUIRED PATHS "${_medforge_qt_cmake}" NO_DEFAULT_PATH)
        find_package(Qt6OpenGLWidgets REQUIRED PATHS "${_medforge_qt_cmake}" NO_DEFAULT_PATH)
        target_link_libraries(medforge_ui PUBLIC Qt6::OpenGL Qt6::OpenGLWidgets)
    endif()
    target_link_libraries(medforge_ui PUBLIC
        VTK::CommonCore
        VTK::FiltersSources
        VTK::RenderingCore
        VTK::RenderingOpenGL2
        VTK::RenderingAnnotation
        VTK::InteractionStyle
        VTK::GUISupportQt
    )
    vtk_module_autoinit(
        TARGETS medforge_ui
        MODULES VTK::RenderingOpenGL2 VTK::GUISupportQt VTK::InteractionStyle
    )
    if(VTK_DIR)
        get_filename_component(MEDFORGE_VTK_BIN "${VTK_DIR}/../../../bin" ABSOLUTE)
        set(MEDFORGE_VTK_BIN "${MEDFORGE_VTK_BIN}" PARENT_SCOPE)
    endif()
endif()
