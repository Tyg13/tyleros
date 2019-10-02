#!/bin/bash

set -e

cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain/x86_64.cmake ..
make $*
