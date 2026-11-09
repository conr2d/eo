# Go/C++ behavioral parity cases. See README.md in this directory.

find_program(EO_GO_EXECUTABLE go)
if(EO_GO_EXECUTABLE)
  # Reject shims or wrappers that exist but cannot run.
  execute_process(COMMAND ${EO_GO_EXECUTABLE} version RESULT_VARIABLE eo_go_version_result OUTPUT_QUIET ERROR_QUIET)
  if(NOT eo_go_version_result EQUAL 0)
    message(STATUS "Go at ${EO_GO_EXECUTABLE} does not run; skipping Go reference tests")
    set(EO_GO_EXECUTABLE "")
  endif()
endif()
if(NOT EO_GO_EXECUTABLE)
  message(STATUS "Go not found: parity tests compare C++ against golden output only")
endif()

set(EO_PARITY_COMPARE ${CMAKE_CURRENT_SOURCE_DIR}/compare.cmake)

# eo_add_parity_test(<test> <case> <known-gap-file> <timeout> <environment>
# [GO])
function(eo_add_parity_test test name known_gap timeout environment)
  set(case_dir ${CMAKE_CURRENT_SOURCE_DIR}/${name})
  if(ARGN STREQUAL "GO")
    set(side -DGO=${EO_GO_EXECUTABLE} -DCASE_NAME=${name} -DPARITY_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR})
  else()
    set(side -DPROGRAM=$<TARGET_FILE:eo_parity_${name}> -DKNOWN_GAP=${case_dir}/${known_gap})
  endif()
  add_test(
    NAME ${test}
    COMMAND ${CMAKE_COMMAND} ${side} -DCASE_DIR=${case_dir} -DTIMEOUT=${timeout} -P ${EO_PARITY_COMPARE})
  if(environment)
    set_tests_properties(${test} PROPERTIES ENVIRONMENT "${environment}")
  endif()
endfunction()

# eo_add_parity_case(<name> [TIMEOUT <seconds>] [ENVIRONMENT <VAR=value>...])
#
# Registers parity.<name>.cpp (C++ translation vs golden output) and, when Go is
# available,
# parity.<name>.go (Go reference vs golden output). ENVIRONMENT applies to the
# C++ side. A
# <name>/KNOWN_GAP file marks the C++ side as a known gap; see compare.cmake.
function(eo_add_parity_case name)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" "TIMEOUT" "ENVIRONMENT")
  if(NOT arg_TIMEOUT)
    set(arg_TIMEOUT 60)
  endif()

  add_executable(eo_parity_${name} ${CMAKE_CURRENT_SOURCE_DIR}/${name}/main.cpp)
  target_link_libraries(eo_parity_${name} PRIVATE eo::main)

  eo_add_parity_test(parity.${name}.cpp ${name} KNOWN_GAP ${arg_TIMEOUT} "${arg_ENVIRONMENT}")
  if(EO_GO_EXECUTABLE)
    eo_add_parity_test(parity.${name}.go ${name} "" ${arg_TIMEOUT} "" GO)
  endif()
endfunction()

# eo_add_parity_variant(<name> <variant> [TIMEOUT <seconds>] [ENVIRONMENT
# <VAR=value>...]
#                       [GO_ENVIRONMENT <VAR=value>...])
#
# Re-runs an existing case under a different environment as
# parity.<name>.cpp.<variant> and, when
# Go is available, parity.<name>.go.<variant>. A <name>/KNOWN_GAP.<variant> file
# marks the C++ side
# as a known gap.
function(eo_add_parity_variant name variant)
  cmake_parse_arguments(PARSE_ARGV 2 arg "" "TIMEOUT" "ENVIRONMENT;GO_ENVIRONMENT")
  if(NOT arg_TIMEOUT)
    set(arg_TIMEOUT 60)
  endif()

  eo_add_parity_test(parity.${name}.cpp.${variant} ${name} KNOWN_GAP.${variant} ${arg_TIMEOUT} "${arg_ENVIRONMENT}")
  if(EO_GO_EXECUTABLE)
    eo_add_parity_test(parity.${name}.go.${variant} ${name} "" ${arg_TIMEOUT} "${arg_GO_ENVIRONMENT}" GO)
  endif()
endfunction()

eo_add_parity_case(channel_buffered)
eo_add_parity_case(defer_args)
eo_add_parity_case(defer_in_loop)
eo_add_parity_case(goroutine_panic TIMEOUT 5)
eo_add_parity_case(map_println)
eo_add_parity_case(panic_exit)
eo_add_parity_case(println_operands)
eo_add_parity_case(select_default)
eo_add_parity_case(shared_variable)
# Pin the worker count so the base case does not depend on the host's CPU count.
eo_add_parity_case(waitgroup_results ENVIRONMENT EOMAXPROCS=4)
eo_add_parity_variant(waitgroup_results maxprocs1 TIMEOUT 5 ENVIRONMENT EOMAXPROCS=1 GO_ENVIRONMENT GOMAXPROCS=1)
