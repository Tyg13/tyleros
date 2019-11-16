#!/bin/bash

set -e
if [[ "$DEBUG" == "1" ]]; then
    set -x
fi

if [[ "$1" == "clean" ]]; then
    rm -rf build/* bin/* sysroot/*
fi

cd build

cmake .. \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
    -DCMAKE_TOOLCHAIN_FILE=../toolchain/x86_64.cmake \

make -j$(($(nproc) + 1)) $*
