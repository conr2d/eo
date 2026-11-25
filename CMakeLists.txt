cmake_minimum_required(VERSION 3.25)

project(eo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(BOOST_ASIO_HAS_STD_INVOKE_RESULT)

# boost::asio::any_completion_handler, which channels and sync::WaitGroup use, needs Boost 1.81. CI
# tests 1.83, Ubuntu 24.04's, and the latest release, so older versions are not accepted.
find_package(Boost 1.83 REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Threads REQUIRED)

# Builds everything with -fsanitize=<list>, e.g. address,undefined or thread.
set(EO_SANITIZE "" CACHE STRING "Sanitizers to build with, passed to -fsanitize=")
if(EO_SANITIZE)
  add_compile_options(-fsanitize=${EO_SANITIZE} -fno-sanitize-recover=all -fno-omit-frame-pointer)
  add_link_options(-fsanitize=${EO_SANITIZE})
endif()

add_subdirectory(src)

# A project that adds Eo with add_subdirectory(), such as a port that links eo::eo, gets the library
# alone, without Eo's examples and tests.
if(PROJECT_IS_TOP_LEVEL)
  include(CTest)
  add_subdirectory(examples)
  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif()
endif()
