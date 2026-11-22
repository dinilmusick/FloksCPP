#!/usr/bin/env sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
for compiler in g++ clang++; do
  for mode in Debug Release; do
    build="$HERE/.verify-${compiler}-${mode}"
    CXX="$compiler" cmake -S "$HERE" -B "$build" -DCMAKE_BUILD_TYPE="$mode" -DBUILD_TESTING=ON -DFLOKSCPP_STRICT_WARNINGS=ON
    cmake --build "$build" --parallel 2
    ctest --test-dir "$build" --output-on-failure
  done
done
