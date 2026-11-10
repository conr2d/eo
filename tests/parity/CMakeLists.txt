# Go/C++ behavioral parity cases. See README.md in this directory.

find_program(EO_GO_EXECUTABLE go)
if(EO_GO_EXECUTABLE)
  # Reject shims that cannot run and toolchains older than the go.mod version.
  # Run outside the module, so an old toolchain reports its version instead of
  # refusing go.mod.
  set(eo_go_local ${CMAKE_COMMAND} -E env GOTOOLCHAIN=local)
  execute_process(
    COMMAND ${eo_go_local} ${EO_GO_EXECUTABLE} env GOVERSION
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    RESULT_VARIABLE eo_go_result
    OUTPUT_VARIABLE eo_go_version ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(eo_go_minor "")
  if(eo_go_version MATCHES "go([0-9]+\\.[0-9]+)")
    set(eo_go_minor ${CMAKE_MATCH_1})
  endif()
  if(NOT eo_go_result EQUAL 0)
    message(STATUS "Go at ${EO_GO_EXECUTABLE} does not run; skipping Go reference tests")
    set(EO_GO_EXECUTABLE "")
  elseif(NOT eo_go_minor OR eo_go_minor VERSION_LESS 1.22)
    message(
      STATUS "Go at ${EO_GO_EXECUTABLE} reports '${eo_go_version}', not go1.22 or newer; skipping Go reference tests")
    set(EO_GO_EXECUTABLE "")
  endif()
endif()
if(NOT EO_GO_EXECUTABLE)
  message(STATUS "Go not found: parity tests compare C++ against golden output only")
endif()

set(EO_PARITY_COMPARE ${CMAKE_CURRENT_SOURCE_DIR}/compare.cmake)

# eo_add_parity_test(<test> <case> <marker> <timeout> <environment> [GO])
function(eo_add_parity_test test name known_gap timeout environment)
  set(case_dir ${CMAKE_CURRENT_SOURCE_DIR}/${name})
  if(ARGN STREQUAL "GO")
    set(side -DGO=${EO_GO_EXECUTABLE} -DCASE_NAME=${name} -DPARITY_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DPROGRAM=${CMAKE_CURRENT_BINARY_DIR}/${test}-bin${CMAKE_EXECUTABLE_SUFFIX})
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
# Registers parity.<name>.cpp and, with Go, parity.<name>.go. ENVIRONMENT
# applies to the C++ side. <name>/KNOWN_GAP marks a known C++ gap.
function(eo_add_parity_case name)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" "TIMEOUT" "ENVIRONMENT")
  if(NOT arg_TIMEOUT)
    set(arg_TIMEOUT 60)
  endif()

  set_property(GLOBAL APPEND PROPERTY EO_PARITY_CASES ${name})
  add_executable(eo_parity_${name} ${CMAKE_CURRENT_SOURCE_DIR}/${name}/main.cpp)
  target_link_libraries(eo_parity_${name} PRIVATE eo::main)

  eo_add_parity_test(parity.${name}.cpp ${name} KNOWN_GAP ${arg_TIMEOUT} "${arg_ENVIRONMENT}")
  if(EO_GO_EXECUTABLE)
    eo_add_parity_test(parity.${name}.go ${name} "" ${arg_TIMEOUT} "" GO)
  endif()
endfunction()

# eo_add_parity_variant(<name> <variant> [TIMEOUT <seconds>]
#   [ENVIRONMENT <VAR=value>...] [GO_ENVIRONMENT <VAR=value>...])
#
# Re-runs a case as parity.<name>.cpp.<variant> and parity.<name>.go.<variant>.
# <name>/KNOWN_GAP.<variant> marks a known C++ gap.
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
eo_add_parity_case(exit_from_goroutine)
eo_add_parity_case(goroutine_panic)
eo_add_parity_case(main_returns_early TIMEOUT 10)
eo_add_parity_case(map_println)
eo_add_parity_case(panic_exit)
eo_add_parity_case(panic_runs_defers)
eo_add_parity_case(plain_goroutine_panic)
eo_add_parity_case(println_before_exit)
eo_add_parity_case(println_operands)
eo_add_parity_case(recover_named_result)
eo_add_parity_case(recover_runtime_error)
eo_add_parity_case(repanic_in_defer)
eo_add_parity_case(select_default)
eo_add_parity_case(shared_variable)
# Pin the worker count so the base case does not depend on the host's CPU count.
eo_add_parity_case(waitgroup_results ENVIRONMENT EOMAXPROCS=4)
eo_add_parity_variant(waitgroup_results maxprocs1 TIMEOUT 10 ENVIRONMENT EOMAXPROCS=1 GO_ENVIRONMENT GOMAXPROCS=1)

# A case directory missing from the list above gets a failing test, so CTest
# reports it without breaking the build.
file(GLOB eo_parity_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*/main.go)
get_property(eo_parity_cases GLOBAL PROPERTY EO_PARITY_CASES)
foreach(source IN LISTS eo_parity_sources)
  get_filename_component(eo_parity_name ${source} DIRECTORY)
  if(NOT eo_parity_name IN_LIST eo_parity_cases)
    message(WARNING "tests/parity/${eo_parity_name} has no eo_add_parity_case() entry")
    add_test(NAME parity.${eo_parity_name}.unregistered COMMAND ${CMAKE_COMMAND} -E false)
  endif()
endforeach()
