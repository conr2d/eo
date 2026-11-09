# Runs one side of a parity case and compares it with the Go-derived golden
# output in CASE_DIR (expected.stdout, optional expected.exitcode).
#
# C++ side: -DPROGRAM=<executable>
# Go side: -DGO=<go executable> -DCASE_NAME=<name> -DPARITY_DIR=<dir>
# -DWORK_DIR=<dir>
# Optional: -DTIMEOUT=<seconds> (default 60)
# -DKNOWN_GAP=<file> expected-failure marker; its first line names the one
# mismatch that must occur: exitcode, stdout, or timeout.
#
# Outcomes are decided here rather than with CTest WILL_FAIL, so that a missing
# binary, a crash, or a different mismatch never passes as a known gap, and a
# marker added or removed takes effect without reconfiguring.

if(NOT DEFINED TIMEOUT)
  set(TIMEOUT 60)
endif()

if(DEFINED GO)
  set(PROGRAM "${WORK_DIR}/${CASE_NAME}")
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

execute_process(
  COMMAND "${PROGRAM}"
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

# Classify what went wrong. A timeout or a signal makes the output meaningless,
# so it is reported on its own.
set(mismatches "")
set(details "")
if(actual_exitcode MATCHES "timeout")
  list(APPEND mismatches timeout)
  string(APPEND details "timed out after ${TIMEOUT}s\n")
  # Output written before the hang must still be a prefix of the expected output.
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
  if(mismatches STREQUAL "")
    message(FATAL_ERROR "known gap closed: output now matches Go. Delete ${KNOWN_GAP}.")
  endif()
  if(NOT mismatches STREQUAL declared)
    message(FATAL_ERROR "known gap declares '${declared}' but got '${mismatches}'\n${details}")
  endif()
  message(STATUS "known gap reproduced (${declared})")
elseif(NOT mismatches STREQUAL "")
  message(FATAL_ERROR "${details}")
endif()
