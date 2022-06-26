#!/usr/bin/env bash

if [[ $# -eq 0 ]];
then
  paths="src"
else
  paths="$@"
fi

which docker > /dev/null
docker_not_found=$?

if [[ $docker_not_found -eq 0 ]];
then
  docker run -dt -v $(pwd):$(pwd) -w $(pwd) --name noir_format --entrypoint /bin/bash ghcr.io/jidicula/clang-format:14 > /dev/null
  formatter="docker exec noir_format /usr/bin/clang-format -i --style=file"
else
  formatter="clang-format -i --style=file"
fi

for path in $paths
do
  if [[ -d "$path" ]]
  then
    find "$path" \( -name "*.h" -o -name "*.cpp" \) -exec $formatter {} \;
  else
    $formatter "$path"
  fi
  if [[ $? -ne 0 ]];
  then
    exit $?
  fi
done

if [[ $docker_not_found -eq 0 ]];
then
  docker stop noir_format > /dev/null
  docker rm noir_format > /dev/null
fi
