# Builds TARGET in BUILD_DIR and passes only if the build fails with an ambiguity error for every name
# in NAMES. The C locale keeps the compiler's messages in English with ASCII quotes.
execute_process(
  COMMAND
    ${CMAKE_COMMAND}
    -E
    env
    LC_ALL=C
    LANG=C
    ${CMAKE_COMMAND}
    --build
    ${BUILD_DIR}
    --target
    ${TARGET}
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output
  RESULT_VARIABLE result)
if(result EQUAL 0)
  message(FATAL_ERROR "${TARGET} compiled, but it must not:\n${output}")
endif()
foreach(name IN LISTS NAMES)
  if(NOT output MATCHES "reference to '${name}' is ambiguous")
    message(FATAL_ERROR "no ambiguity error for '${name}':\n${output}")
  endif()
endforeach()
