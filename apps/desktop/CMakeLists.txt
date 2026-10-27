add_executable(medforge main.cpp)
target_link_libraries(medforge PRIVATE medforge::ui)
medforge_set_warnings(medforge)
