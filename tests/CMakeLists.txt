add_executable(eo_smoke smoke.cpp)
target_link_libraries(eo_smoke PRIVATE eo::eo)

add_test(NAME eo.smoke COMMAND eo_smoke)

add_executable(eo_channel channel.cpp)
target_link_libraries(eo_channel PRIVATE eo::eo)

add_test(NAME eo.channel COMMAND eo_channel)

add_executable(eo_select select.cpp)
target_link_libraries(eo_select PRIVATE eo::eo)

add_test(NAME eo.select COMMAND eo_select)

add_executable(eo_select_default_position select_default_position.cpp)
target_link_libraries(eo_select_default_position PRIVATE eo::eo)

add_test(NAME eo.select_default_position COMMAND eo_select_default_position)

add_executable(eo_context context.cpp)
target_link_libraries(eo_context PRIVATE eo::eo)

add_test(NAME eo.context COMMAND eo_context)

add_executable(eo_error error.cpp)
target_link_libraries(eo_error PRIVATE eo::eo)

add_test(NAME eo.error COMMAND eo_error)

add_executable(eo_invoke invoke.cpp)
target_link_libraries(eo_invoke PRIVATE eo::eo)

add_test(NAME eo.invoke COMMAND eo_invoke)

add_executable(eo_runtime runtime.cpp)
target_link_libraries(eo_runtime PRIVATE eo::eo)

add_test(NAME eo.runtime COMMAND eo_runtime)
add_test(NAME eo.runtime_two_workers COMMAND eo_runtime)
set_tests_properties(eo.runtime_two_workers PROPERTIES ENVIRONMENT "EOMAXPROCS=2")
add_test(NAME eo.runtime_zero_workers COMMAND eo_runtime)
set_tests_properties(eo.runtime_zero_workers PROPERTIES ENVIRONMENT "EOMAXPROCS=0")
add_test(NAME eo.runtime_invalid_workers COMMAND eo_runtime)
set_tests_properties(eo.runtime_invalid_workers PROPERTIES ENVIRONMENT "EOMAXPROCS=invalid")
