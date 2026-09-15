add_executable(eo_smoke smoke.cpp)
target_link_libraries(eo_smoke PRIVATE eo::eo)

add_test(NAME eo.smoke COMMAND eo_smoke)

add_executable(eo_channel channel.cpp)
target_link_libraries(eo_channel PRIVATE eo::eo)

add_test(NAME eo.channel COMMAND eo_channel)

add_executable(eo_channel_status channel_status.cpp)
target_link_libraries(eo_channel_status PRIVATE eo::eo)

add_test(NAME eo.channel_status COMMAND eo_channel_status)

add_executable(eo_select select.cpp)
target_link_libraries(eo_select PRIVATE eo::eo)

add_test(NAME eo.select COMMAND eo_select)

add_executable(eo_select_canonical select_canonical.cpp)
target_link_libraries(eo_select_canonical PRIVATE eo::eo)

add_test(NAME eo.select_canonical COMMAND eo_select_canonical)

add_executable(eo_context context.cpp)
target_link_libraries(eo_context PRIVATE eo::eo)

add_test(NAME eo.context COMMAND eo_context)

add_executable(eo_error error.cpp)
target_link_libraries(eo_error PRIVATE eo::eo)

add_test(NAME eo.error COMMAND eo_error)

add_executable(eo_invoke invoke.cpp)
target_link_libraries(eo_invoke PRIVATE eo::eo)

add_test(NAME eo.invoke COMMAND eo_invoke)

add_executable(eo_os os.cpp)
target_link_libraries(eo_os PRIVATE eo::eo)

add_test(NAME eo.os COMMAND eo_os)

add_executable(eo_math_rand math_rand.cpp)
target_link_libraries(eo_math_rand PRIVATE eo::eo)

add_test(NAME eo.math_rand COMMAND eo_math_rand)

add_executable(eo_fmt fmt.cpp)
target_link_libraries(eo_fmt PRIVATE eo::eo)

add_test(NAME eo.fmt COMMAND eo_fmt)

add_executable(eo_waitgroup waitgroup.cpp)
target_link_libraries(eo_waitgroup PRIVATE eo::eo)

add_test(NAME eo.waitgroup COMMAND eo_waitgroup)

add_executable(eo_timer timer.cpp)
target_link_libraries(eo_timer PRIVATE eo::eo)

add_test(NAME eo.timer COMMAND eo_timer)

add_executable(eo_ticker ticker.cpp)
target_link_libraries(eo_ticker PRIVATE eo::eo)

add_test(NAME eo.ticker COMMAND eo_ticker)

add_executable(eo_runtime runtime.cpp)
target_link_libraries(eo_runtime PRIVATE eo::eo)

add_test(NAME eo.runtime COMMAND eo_runtime)
add_test(NAME eo.runtime_two_workers COMMAND eo_runtime)
set_tests_properties(eo.runtime_two_workers PROPERTIES ENVIRONMENT "EOMAXPROCS=2")
add_test(NAME eo.runtime_zero_workers COMMAND eo_runtime)
set_tests_properties(eo.runtime_zero_workers PROPERTIES ENVIRONMENT "EOMAXPROCS=0")
add_test(NAME eo.runtime_invalid_workers COMMAND eo_runtime)
set_tests_properties(eo.runtime_invalid_workers PROPERTIES ENVIRONMENT "EOMAXPROCS=invalid")
