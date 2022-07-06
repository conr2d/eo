cmake_minimum_required(VERSION 3.16)

project(eo)

set(CMAKE_CXX_STANDARD 20)
add_compile_definitions(BOOST_ASIO_HAS_STD_INVOKE_RESULT)

include(cmake_common.txt)

# boost::asio::experimental::awaitable_operators doesn't work with GCC
compiler_minimum_required("GNU" UNSUPPORTED)

compiler_minimum_required("Clang" 10.0)
compiler_minimum_required("AppleClang" 10.0)

option(USE_CONAN "use conan to setup build dependencies" ON)
option(CHECK_CONNECTION "check the network connection status" ON)

set(CONAN_PACKAGES
  fmt/8.1.1
  scope-lite/0.2.0
)
if(UNIX AND NOT APPLE)
  set(CONAN_PACKAGES
    ${CONAN_PACKAGES}
    boost/1.78.0
  )
endif()

setup_conan()
setup_macos_path()

find_package(Boost REQUIRED)
find_package(fmt REQUIRED)
find_package(scope-lite REQUIRED)

add_subdirectory(src)
add_subdirectory(examples)
