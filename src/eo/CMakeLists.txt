add_library(eo STATIC context.cpp math/rand.cpp os.cpp runtime.cpp)

target_include_directories(eo PUBLIC ${PROJECT_SOURCE_DIR}/src ${Boost_INCLUDE_DIRS})
target_link_libraries(eo PUBLIC fmt::fmt Threads::Threads)

add_library(eo_main STATIC main.cpp)
target_link_libraries(eo_main PUBLIC eo)

add_library(eo::eo ALIAS eo)
add_library(eo::main ALIAS eo_main)
