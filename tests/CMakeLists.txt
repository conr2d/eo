add_executable(eo_smoke smoke.cpp)
target_link_libraries(eo_smoke PRIVATE eo::eo)

add_test(NAME eo.smoke COMMAND eo_smoke)

add_executable(eo_channel channel.cpp)
target_link_libraries(eo_channel PRIVATE eo::eo)

add_test(NAME eo.channel COMMAND eo_channel)
