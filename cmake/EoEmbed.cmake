# eo_embed(<target> [NAMESPACE <namespace>] VARIABLE <variable> BASE_DIR <dir> FILES <file>...)
#
# Compiles the files that a //go:embed directive of a Go variable names into <target>, as the Go
# toolchain compiles them into the program. Each <file> is a path relative to <dir>, the directory of
# the Go package, and is also the name of the file in an embed.FS. The generated source defines, in
# <namespace>, or in the global namespace of package main when NAMESPACE is omitted, the function
# _eo_embed_<variable>(), which returns the files as a
# std::span<const eo::embed::file>, and which the translation of the variable calls (see the
# //go:embed directives section of docs/TRANSLATION_RULES.md). The source is regenerated when a file
# changes.
set(EO_EMBED_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/eo_embed_generate.cmake)

function(eo_embed target)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" "NAMESPACE;VARIABLE;BASE_DIR" "FILES")
  foreach(required VARIABLE BASE_DIR FILES)
    if(NOT arg_${required})
      message(FATAL_ERROR "eo_embed: ${required} is required")
    endif()
  endforeach()

  set(scope ${arg_NAMESPACE})
  if(NOT scope)
    set(scope main)
  endif()
  set(output ${CMAKE_CURRENT_BINARY_DIR}/eo_embed/${target}/${scope}/${arg_VARIABLE}.cpp)
  set(depends "")
  foreach(file IN LISTS arg_FILES)
    list(APPEND depends ${arg_BASE_DIR}/${file})
  endforeach()
  # The file list goes to the script as one argument, so its separator is not the list separator.
  string(REPLACE ";" "|" files "${arg_FILES}")
  add_custom_command(
    OUTPUT ${output}
    COMMAND
      ${CMAKE_COMMAND}
      -DOUTPUT=${output}
      "-DNAMESPACE=${arg_NAMESPACE}"
      -DVARIABLE=${arg_VARIABLE}
      -DBASE_DIR=${arg_BASE_DIR}
      "-DFILES=${files}"
      -P
      ${EO_EMBED_SCRIPT}
    DEPENDS ${depends} ${EO_EMBED_SCRIPT}
    COMMENT "Embedding ${arg_VARIABLE} in ${target}"
    VERBATIM)
  target_sources(${target} PRIVATE ${output})
endfunction()
