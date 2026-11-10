#!/usr/bin/env bash
# Regenerate expected.stdout / expected.exitcode from the Go reference programs.
# Usage: tests/parity/update-golden.sh [case...]
set -euo pipefail

cd "$(dirname "$0")"
go=${GO:-go}
export GOTOOLCHAIN=local
# Run with a 60s limit when timeout or gtimeout exists. A function rather than
# an array, because bash 3.2 with set -u rejects an empty array expansion.
if command -v timeout >/dev/null; then
  limit=timeout
elif command -v gtimeout >/dev/null; then
  limit=gtimeout
else
  limit=
fi
run_limited() {
  if [[ -n $limit ]]; then
    "$limit" 60 "$@"
  else
    "$@"
  fi
}
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT

if [[ $# -eq 0 ]]; then
  for file in */main.go; do
    set -- "$@" "${file%/main.go}"
  done
fi

for name in "$@"; do
  "$go" build -o "$out/$name" "./$name/main.go"
  set +e
  run_limited "$out/$name" >"$out/$name.stdout"
  rc=$?
  set -e
  if [[ -n $limit && $rc -eq 124 ]]; then
    echo "$name: timed out; golden files left unchanged" >&2
    exit 1
  fi
  mv "$out/$name.stdout" "$name/expected.stdout"
  if [[ $rc -eq 0 ]]; then
    rm -f "$name/expected.exitcode"
  else
    echo "$rc" >"$name/expected.exitcode"
  fi
  echo "$name: exit $rc"
done
