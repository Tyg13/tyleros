#!/bin/bash

set -e

cd build

cmake .. \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
    -DCMAKE_TOOLCHAIN_FILE=../toolchain/x86_64.cmake \

make -j$(($(nproc) + 1)) $*
