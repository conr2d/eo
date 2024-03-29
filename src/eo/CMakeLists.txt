add_library(eo STATIC
  context.cpp
  math/rand.cpp
  os.cpp
  runtime.cpp
)
target_include_directories(eo PUBLIC ${PROJECT_SOURCE_DIR}/src ${Boost_INCLUDE_DIR})
target_link_libraries(eo
  fmt::fmt
  nonstd::scope-lite
)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
  target_link_libraries(eo Threads::Threads)
endif()

add_library(eo_main STATIC main.cpp)
target_link_libraries(eo_main eo)

add_library(eo::eo ALIAS eo)
add_library(eo::main ALIAS eo_main)
