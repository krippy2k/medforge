add_library(medforge_rendering STATIC
    src/CameraPose.cpp
    src/Module.cpp
)
add_library(medforge::rendering ALIAS medforge_rendering)
target_include_directories(medforge_rendering PUBLIC include)
target_link_libraries(medforge_rendering PUBLIC medforge::core)
target_compile_features(medforge_rendering PUBLIC cxx_std_20)
set_target_properties(medforge_rendering PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
medforge_set_core_warnings(medforge_rendering)
