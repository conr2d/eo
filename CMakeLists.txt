cmake_minimum_required(VERSION 3.25)

project(eo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(BOOST_ASIO_HAS_STD_INVOKE_RESULT)

# boost::asio::any_completion_handler, used by sync::WaitGroup, needs Boost 1.81.
find_package(Boost 1.81 REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Threads REQUIRED)

# Builds everything with -fsanitize=<list>, e.g. address,undefined or thread.
set(EO_SANITIZE "" CACHE STRING "Sanitizers to build with, passed to -fsanitize=")
if(EO_SANITIZE)
  add_compile_options(-fsanitize=${EO_SANITIZE} -fno-sanitize-recover=all -fno-omit-frame-pointer)
  add_link_options(-fsanitize=${EO_SANITIZE})
endif()

include(CTest)

add_subdirectory(src)
add_subdirectory(examples)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
