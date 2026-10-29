add_library(medforge_geometry STATIC
    src/Module.cpp
    src/Surface.cpp
    src/Measure.cpp
    src/MeshProcess.cpp
)
add_library(medforge::geometry ALIAS medforge_geometry)
target_include_directories(medforge_geometry PUBLIC include)
target_link_libraries(medforge_geometry PUBLIC medforge::core)
target_compile_features(medforge_geometry PUBLIC cxx_std_20)
set_target_properties(medforge_geometry PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
medforge_set_core_warnings(medforge_geometry)
