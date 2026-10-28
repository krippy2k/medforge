medforge_add_test(core_coordinates_test
    SOURCES core/CoordinatesTest.cpp
    LIBS medforge::core
)
medforge_add_test(core_entity_id_test
    SOURCES core/EntityIdTest.cpp
    LIBS medforge::core
)
medforge_add_test(core_cancellation_test
    SOURCES core/CancellationTest.cpp
    LIBS medforge::core
)
medforge_add_test(core_command_history_test
    SOURCES core/CommandHistoryTest.cpp
    LIBS medforge::core
)
medforge_add_test(core_synthetic_volume_test
    SOURCES core/SyntheticVolumeTest.cpp
    LIBS medforge::core
)
target_compile_definitions(core_synthetic_volume_test PRIVATE MEDFORGE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

medforge_add_test(imaging_ct_series_test
    SOURCES imaging/CtSeriesTest.cpp
    LIBS medforge::imaging
)
medforge_add_test(imaging_dicom_read_test
    SOURCES imaging/DicomReadTest.cpp
    LIBS medforge::imaging
)
medforge_add_test(imaging_ct_volume_test
    SOURCES imaging/CtVolumeTest.cpp
    LIBS medforge::imaging
)
medforge_add_test(imaging_ct_import_test
    SOURCES imaging/CtImportTest.cpp
    LIBS medforge::imaging
)
target_compile_definitions(imaging_ct_import_test PRIVATE MEDFORGE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

medforge_add_test(core_modules_test
    SOURCES core/ModulesTest.cpp
    LIBS medforge::core medforge::imaging medforge::geometry medforge::cad medforge::rendering
)

medforge_add_test(ui_segmented_control_test
    SOURCES ui/SegmentedControlTest.cpp
    LIBS medforge::ui
)
medforge_add_test(ui_numeric_editor_test
    SOURCES ui/NumericEditorTest.cpp
    LIBS medforge::ui
)
medforge_add_test(ui_foundation_widgets_test
    SOURCES ui/FoundationWidgetsTest.cpp
    LIBS medforge::ui
)
medforge_add_test(ui_style_test
    SOURCES ui/StyleTest.cpp
    LIBS medforge::ui
)
medforge_add_test(rendering_camera_pose_test
    SOURCES rendering/CameraPoseTest.cpp
    LIBS medforge::rendering
)
medforge_add_test(ui_workspace_actions_test
    SOURCES ui/WorkspaceActionsTest.cpp
    LIBS medforge::ui
)
medforge_add_test(ui_mpr_workspace_test
    SOURCES ui/MprWorkspaceTest.cpp
    LIBS medforge::ui
)
medforge_add_test(ui_workspace_window_test
    SOURCES ui/WorkspaceWindowTest.cpp
    LIBS medforge::ui
)
medforge_add_test(ui_layout_preferences_test
    SOURCES ui/LayoutPreferencesTest.cpp
    LIBS medforge::ui
)
medforge_add_test(ui_screenshot_capture_test
    SOURCES ui/ScreenshotCaptureTest.cpp
    LIBS medforge::ui
)
set_tests_properties(ui_screenshot_capture_test PROPERTIES TIMEOUT 180)
