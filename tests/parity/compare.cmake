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
  # A gap also pins what the C++ side actually does: <marker>.stdout is required
  # for stdout and timeout gaps and compared whenever present; <marker>.exitcode
  # is required for, and only allowed with, an exitcode gap.
  set(gap_stdout_file "${KNOWN_GAP}.stdout")
  set(gap_exitcode_file "${KNOWN_GAP}.exitcode")
  if(declared MATCHES "^(stdout|timeout)$" AND NOT EXISTS "${gap_stdout_file}")
    message(FATAL_ERROR "${KNOWN_GAP} declares ${declared} but ${gap_stdout_file} is missing")
  endif()
  if(declared STREQUAL "exitcode" AND NOT EXISTS "${gap_exitcode_file}")
    message(FATAL_ERROR "${KNOWN_GAP} declares exitcode but ${gap_exitcode_file} is missing")
  endif()
  if(NOT declared STREQUAL "exitcode" AND EXISTS "${gap_exitcode_file}")
    message(FATAL_ERROR "${gap_exitcode_file} exists but ${KNOWN_GAP} does not declare exitcode")
  endif()
  if(mismatches STREQUAL "")
    message(FATAL_ERROR "known gap closed: output now matches Go. Delete ${KNOWN_GAP} and its .stdout/.exitcode files.")
  endif()
  if(NOT mismatches STREQUAL declared)
    message(FATAL_ERROR "known gap declares '${declared}' but got '${mismatches}'\n${details}")
  endif()
  if(EXISTS "${gap_stdout_file}")
    file(READ "${gap_stdout_file}" gap_stdout)
    if(NOT actual_stdout STREQUAL gap_stdout)
      message(
        FATAL_ERROR
          "stdout differs from ${gap_stdout_file}; update it if the new output is right\n--- expected\n${gap_stdout}--- actual\n${actual_stdout}---\n${details}")
    endif()
  endif()
  if(EXISTS "${gap_exitcode_file}")
    file(STRINGS "${gap_exitcode_file}" gap_exitcode LIMIT_COUNT 1)
    if(NOT actual_exitcode STREQUAL gap_exitcode)
      message(
        FATAL_ERROR
          "exit code ${actual_exitcode} differs from ${gap_exitcode_file} (${gap_exitcode}); update it if the new exit code is right\n${details}")
    endif()
  endif()
  message(STATUS "known gap reproduced (${declared})")
elseif(NOT mismatches STREQUAL "")
  message(FATAL_ERROR "${details}")
endif()
