cmake_minimum_required (VERSION 3.14)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# Enable NASM, using CMAKE_LINKER as our linker instead of NASM
set(CAN_USE_ASSEMBLER TRUE)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_LINK_EXECUTABLE "<CMAKE_LINKER> <CMAKE_ASM_NASM_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET>")
enable_language(ASM_NASM)

project(tyleros C CXX ASM_NASM)

set(BINARY_DIR "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BINARY_DIR}")

# Default to Debug if not specified on the command line
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT DEFINED CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++2a -Wall -Werror -Wno-unused-const-variable -mno-red-zone -ffreestanding -fno-exceptions -fno-rtti -mcmodel=kernel -fno-pic")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -Og")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

add_subdirectory(src)
