add_executable(medforge_fixtures main.cpp)
target_link_libraries(medforge_fixtures PRIVATE medforge::core)
medforge_set_core_warnings(medforge_fixtures)
set_target_properties(medforge_fixtures PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
