cmake_minimum_required(VERSION 3.25)

project(eo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(BOOST_ASIO_HAS_STD_INVOKE_RESULT)

find_package(Boost REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Threads REQUIRED)

include(CTest)

add_subdirectory(src)
add_subdirectory(examples)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
