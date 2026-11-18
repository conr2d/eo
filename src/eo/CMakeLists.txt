add_library(eo STATIC context.cpp math/rand.cpp os.cpp runtime.cpp)

target_include_directories(eo PUBLIC ${PROJECT_SOURCE_DIR}/src ${Boost_INCLUDE_DIRS})
target_link_libraries(eo PUBLIC fmt::fmt Threads::Threads)
# Go's signed +, -, * and unary - wrap. -fwrapv makes the plain C++ operators wrap too, and PUBLIC
# passes it to every target that links Eo, so translated code is compiled with it.
target_compile_options(eo PUBLIC -fwrapv)

add_library(eo_main STATIC main.cpp)
target_link_libraries(eo_main PUBLIC eo)

add_library(eo::eo ALIAS eo)
add_library(eo::main ALIAS eo_main)
