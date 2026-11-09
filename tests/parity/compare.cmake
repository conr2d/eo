# Runs one side of a parity case and compares it with the golden output in
# CASE_DIR: expected.stdout and an optional expected.exitcode.
#
# -DPROGRAM=<file>: C++ executable, or the Go output binary when GO is set.
# -DGO=<go> -DCASE_NAME=<name> -DPARITY_DIR=<dir>: build the Go reference.
# -DTIMEOUT=<seconds>: run limit, default 60.
# -DKNOWN_GAP=<file>: optional expected-failure marker; see README.md.
#
# The outcome is decided here rather than with CTest WILL_FAIL, so a missing
# binary, a crash, or an undeclared mismatch never passes as a known gap.

if(NOT DEFINED TIMEOUT)
  set(TIMEOUT 60)
endif()

if(DEFINED GO)
  set(ENV{GOTOOLCHAIN} local)
  execute_process(
    COMMAND "${GO}" build -o "${PROGRAM}" "./${CASE_NAME}/main.go"
    WORKING_DIRECTORY "${PARITY_DIR}"
    RESULT_VARIABLE build_result
    ERROR_VARIABLE build_error
    TIMEOUT 300)
  if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "go build failed (${build_result}):\n${build_error}")
  endif()
endif()

if(NOT EXISTS "${PROGRAM}")
  message(FATAL_ERROR "program not found: ${PROGRAM}")
endif()

# Disable core files for the child where a shell is available, so a crash is
# not slowed by writing one. See README.md about pipe core handlers.
set(run "${PROGRAM}")
if(CMAKE_HOST_UNIX AND EXISTS /bin/sh)
  set(run /bin/sh -c [[ulimit -c 0 && exec "$0"]] "${PROGRAM}")
endif()
execute_process(
  COMMAND ${run}
  WORKING_DIRECTORY "${CASE_DIR}"
  RESULT_VARIABLE actual_exitcode
  OUTPUT_VARIABLE actual_stdout
  ERROR_VARIABLE actual_stderr
  TIMEOUT ${TIMEOUT})

file(READ "${CASE_DIR}/expected.stdout" expected_stdout)
set(expected_exitcode 0)
if(EXISTS "${CASE_DIR}/expected.exitcode")
  file(STRINGS "${CASE_DIR}/expected.exitcode" expected_exitcode LIMIT_COUNT 1)
endif()

# Classify what went wrong. A signal is reported on its own.
set(mismatches "")
set(details "")
if(actual_exitcode MATCHES "timeout")
  list(APPEND mismatches timeout)
  string(APPEND details "timed out after ${TIMEOUT}s\n")
  # Output written before the hang must be a prefix of the expected output.
  string(LENGTH "${actual_stdout}" actual_length)
  string(SUBSTRING "${expected_stdout}" 0 ${actual_length} expected_prefix)
  if(NOT actual_stdout STREQUAL expected_prefix)
    list(APPEND mismatches stdout)
    string(APPEND details "stdout before timeout is not a prefix of expected\n--- actual\n${actual_stdout}---\n")
  endif()
elseif(NOT actual_exitcode MATCHES "^[0-9]+$")
  list(APPEND mismatches crash)
  string(APPEND details "abnormal termination: ${actual_exitcode}\n")
else()
  if(NOT actual_exitcode EQUAL expected_exitcode)
    list(APPEND mismatches exitcode)
    string(APPEND details "exit code mismatch: expected ${expected_exitcode}, got ${actual_exitcode}\n")
  endif()
  if(NOT actual_stdout STREQUAL expected_stdout)
    list(APPEND mismatches stdout)
    string(APPEND details "stdout mismatch\n--- expected\n${expected_stdout}--- actual\n${actual_stdout}---\n")
  endif()
endif()

if(NOT actual_stderr STREQUAL "")
  string(APPEND details "stderr:\n${actual_stderr}")
endif()

if(DEFINED KNOWN_GAP AND EXISTS "${KNOWN_GAP}")
  file(STRINGS "${KNOWN_GAP}" declared LIMIT_COUNT 1)
  if(NOT declared MATCHES "^(exitcode|stdout|timeout)$")
    message(FATAL_ERROR "${KNOWN_GAP}: first line must be exitcode, stdout, or timeout, got '${declared}'")
  endif()
  # A timeout gap also declares the exact output written before the hang.
  set(timeout_stdout_file "${KNOWN_GAP}.stdout")
  if(declared STREQUAL "timeout" AND NOT EXISTS "${timeout_stdout_file}")
    message(FATAL_ERROR "${KNOWN_GAP} declares timeout but ${timeout_stdout_file} is missing")
  endif()
  if(NOT declared STREQUAL "timeout" AND EXISTS "${timeout_stdout_file}")
    message(FATAL_ERROR "${timeout_stdout_file} exists but ${KNOWN_GAP} does not declare timeout")
  endif()
  if(mismatches STREQUAL "")
    message(FATAL_ERROR "known gap closed: output now matches Go. Delete ${KNOWN_GAP}.")
  endif()
  if(NOT mismatches STREQUAL declared)
    message(FATAL_ERROR "known gap declares '${declared}' but got '${mismatches}'\n${details}")
  endif()
  if(declared STREQUAL "timeout")
    file(READ "${timeout_stdout_file}" timeout_stdout)
    if(NOT actual_stdout STREQUAL timeout_stdout)
      message(
        FATAL_ERROR
          "stdout before timeout differs from ${timeout_stdout_file}\n--- expected\n${timeout_stdout}--- actual\n${actual_stdout}---\n${details}")
    endif()
  endif()
  message(STATUS "known gap reproduced (${declared})")
elseif(NOT mismatches STREQUAL "")
  message(FATAL_ERROR "${details}")
endif()
