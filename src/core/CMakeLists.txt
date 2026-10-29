add_library(medforge_core STATIC
    src/Cancellation.cpp
    src/CommandHistory.cpp
    src/Coordinates.cpp
    src/EntityId.cpp
    src/Module.cpp
    src/Project.cpp
    src/SyntheticVolume.cpp
)
add_library(medforge::core ALIAS medforge_core)

target_include_directories(medforge_core PUBLIC include)
target_compile_features(medforge_core PUBLIC cxx_std_20)
set_target_properties(medforge_core PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
medforge_set_core_warnings(medforge_core)
