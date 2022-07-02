cmake_minimum_required(VERSION 3.16)

project(eo)

set(CMAKE_CXX_STANDARD 20)
add_compile_definitions(BOOST_ASIO_HAS_STD_INVOKE_RESULT)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.0)
    message(FATAL_ERROR "GCC version 11 or later is required")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
    message(FATAL_ERROR "Clang version 10 or later is required")
  endif()
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})

if(UNIX AND NOT APPLE)
  execute_process(COMMAND ping conan.io -c 2 -w 1000 RESULT_VARIABLE NO_CONNECTION OUTPUT_QUIET ERROR_QUIET)
elseif(APPLE)
  execute_process(COMMAND ping conan.io -c 2 -W 1000 RESULT_VARIABLE NO_CONNECTION OUTPUT_QUIET ERROR_QUIET)
endif()

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conan.cmake" AND NOT NO_CONNECTION)
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/0.18.1/conan.cmake" "${CMAKE_CURRENT_BINARY_DIR}/conan.cmake" TLS_VERIFY ON)
endif()

include(conan)

set(CONAN_PACKAGES
  date/3.0.1
  fmt/8.1.1
  scope-lite/0.2.0
)
if(UNIX AND NOT APPLE)
  set(CONAN_PACKAGES
    ${CONAN_PACKAGES}
    boost/1.78.0
  )
endif()

conan_cmake_configure(REQUIRES ${CONAN_PACKAGES} GENERATORS cmake_find_package)

set(PROJECT_BUILD_TYPE ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)
conan_cmake_autodetect(CONAN_SETTINGS)
set(CMAKE_BUILD_TYPE ${PROJECT_BUILD_TYPE})

# HACK: remove c++ standard version from conan settings to use prebuilt binaries
string(REPLACE ";compiler.cppstd=20" "" CONAN_SETTINGS "${CONAN_SETTINGS}")
if(NOT NO_CONNECTION)
  conan_cmake_install(PATH_OR_REFERENCE . BUILD missing REMOTE conancenter SETTINGS ${CONAN_SETTINGS})
endif()

if(APPLE)
  if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(BREW_ROOT "/usr/local/opt")
  elseif(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(BREW_ROOT "/opt/homebrew/opt")
  endif()
  set(BOOST_ROOT "${BREW_ROOT}/boost" CACHE STRING "boost root directory")
  set(OPENSSL_ROOT_DIR "${BREW_ROOT}/openssl" CACHE STRING "openssl root directory")
endif()

find_package(Boost REQUIRED)
find_package(date REQUIRED)
find_package(fmt REQUIRED)
find_package(scope-lite REQUIRED)

add_subdirectory(src)
add_subdirectory(examples)
