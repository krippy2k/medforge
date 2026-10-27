add_library(medforge_ui STATIC
    include/medforge/ui/FoundationWidgets.h
    include/medforge/ui/FoundationWindow.h
    include/medforge/ui/GalleryWindow.h
    include/medforge/ui/NumericEditor.h
    include/medforge/ui/SegmentedControl.h
    src/ConfirmDialog.cpp
    src/FoundationWidgets.cpp
    src/FoundationWindow.cpp
    src/GalleryWindow.cpp
    src/Icons.cpp
    src/NumericEditor.cpp
    src/ScreenshotCapture.cpp
    src/SegmentedControl.cpp
    src/Style.cpp
    src/WorkspaceReference.cpp
)
add_library(medforge::ui ALIAS medforge_ui)

target_include_directories(medforge_ui PUBLIC include)
target_link_libraries(medforge_ui PUBLIC medforge::core Qt6::Widgets)
target_compile_features(medforge_ui PUBLIC cxx_std_20)
medforge_set_warnings(medforge_ui)
