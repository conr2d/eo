add_executable(eo_smoke smoke.cpp)
target_link_libraries(eo_smoke PRIVATE eo::eo)

add_test(NAME eo.smoke COMMAND eo_smoke)
