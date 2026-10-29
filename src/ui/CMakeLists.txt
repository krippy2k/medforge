add_library(medforge_ui STATIC
    include/medforge/ui/FoundationWidgets.h
    include/medforge/ui/FoundationWindow.h
    include/medforge/ui/GalleryWindow.h
    include/medforge/ui/NumericEditor.h
    include/medforge/ui/SegmentedControl.h
    include/medforge/ui/SampleViewport.h
    include/medforge/ui/VolumeViewport.h
    include/medforge/ui/WorkspaceActions.h
    include/medforge/ui/MprViewport.h
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
    src/MprViewport.cpp
    src/SampleViewport.cpp
    src/VolumeViewport.cpp
    src/VolumeRayCastDisplay.cpp
    src/Style.cpp
    src/WorkspaceActions.cpp
    src/WorkspaceReference.cpp
    src/WorkspaceWindow.cpp
)
add_library(medforge::ui ALIAS medforge_ui)

target_include_directories(medforge_ui PUBLIC include)
target_link_libraries(medforge_ui PUBLIC medforge::core medforge::imaging medforge::cad medforge::rendering Qt6::Widgets)
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
    CommonDataModel
    FiltersSources
    RenderingCore
    RenderingOpenGL2
    RenderingVolume
    RenderingAnnotation
    InteractionStyle
    GUISupportQt
)
if(VTK_FOUND)
    include(MedForgeVtk)
    medforge_vtk_use_debug_libraries()
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
        VTK::CommonDataModel
        VTK::FiltersSources
        VTK::RenderingCore
        VTK::RenderingOpenGL2
        VTK::RenderingVolume
        VTK::RenderingAnnotation
        VTK::InteractionStyle
        VTK::GUISupportQt
    )
    vtk_module_autoinit(
        TARGETS medforge_ui
        MODULES VTK::RenderingOpenGL2 VTK::RenderingVolume VTK::GUISupportQt VTK::InteractionStyle
    )
    if(VTK_DIR)
        get_filename_component(MEDFORGE_VTK_BIN "${VTK_DIR}/../../../bin" ABSOLUTE)
        set(MEDFORGE_VTK_BIN "${MEDFORGE_VTK_BIN}" PARENT_SCOPE)
    endif()
endif()
