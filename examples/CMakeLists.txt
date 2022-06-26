link_libraries(eo::main)

add_executable(hello-world hello-world.cpp)
add_executable(channels channels.cpp)
add_executable(channel-directions channel-directions.cpp)
add_executable(select select.cpp)
add_executable(non-blocking-channel-operations non-blocking-channel-operations.cpp)
add_executable(waitgroups waitgroups.cpp)
add_executable(mutexes mutexes.cpp)
add_executable(defer defer.cpp)

add_executable(default-selection default-selection.cpp)
