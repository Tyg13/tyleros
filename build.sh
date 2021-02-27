#!/bin/bash

JOBS="-j$(nproc)"

set -e
if [[ "$DEBUG" == "1" ]]; then
    set -x
fi

if [[ "$1" == "clean" ]]; then
    rm -rf build/* bin/* sysroot/*
    [ -f Makefile ] && make "$JOBS" clean
    MAKE_ARGS+=("clean")
    shift
fi

BUILD_TYPE="Debug"

cd build

cmake .. \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
    -DCMAKE_TOOLCHAIN_FILE=../toolchain/x86_64.cmake \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    "$@"

make "$JOBS"
