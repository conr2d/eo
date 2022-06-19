#!/usr/bin/env bash

if [[ $# -eq 0 ]];
then
  paths="src"
else
  paths="$@"
fi

# clang-format-13 wraps enum brace even though its option is set to false.
# Use clang-format-14 shipped with npm `clang-format` package
if [[ "$OSTYPE" == "darwin"* ]];
then
  arch=$(uname -m)
  if [[ "$arch" == "x86_64" ]];
  then
    formatter=/usr/local/lib/node_modules/clang-format/index.js
  elif [[ "$arch" == "arm64" ]];
  then
    formatter=/opt/homebrew/lib/node_modules/clang-format/index.js
  else
    echo "ERROR: unsupported architecture( $arch )"
    exit 1
  fi
else
  formatter=clang-format
fi

for path in $paths
do
  if [[ -d "$path" ]]
  then
    find "$path" \( -name "*.h" -o -name "*.cpp" \) -exec $formatter -i --style=file {} \;
  else
    $formatter -i --style=file "$path"
  fi
  if [[ $? -ne 0 ]];
  then
    exit $?
  fi
done
