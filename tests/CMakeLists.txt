cmake_minimum_required (VERSION 3.14)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(tyleros_tests C CXX)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP NEW
)
FetchContent_MakeAvailable(googletest)
enable_testing()
include(GoogleTest)

add_library(test_c STATIC
    ${CMAKE_SOURCE_DIR}/../src/libc/assert.cpp
    ${CMAKE_SOURCE_DIR}/../src/libc/ctype.cpp
    ${CMAKE_SOURCE_DIR}/../src/libc/errno.cpp
    ${CMAKE_SOURCE_DIR}/../src/libc/stdio.cpp
    ${CMAKE_SOURCE_DIR}/../src/libc/stdlib.cpp
    ${CMAKE_SOURCE_DIR}/../src/libc/string.cpp
    ${CMAKE_SOURCE_DIR}/../src/libc/time.cpp
    ${CMAKE_SOURCE_DIR}/../src/libc/unistd.cpp

    ${CMAKE_SOURCE_DIR}/../src/libc/assert.h
    ${CMAKE_SOURCE_DIR}/../src/libc/ctype.h
    ${CMAKE_SOURCE_DIR}/../src/libc/errno.h
    ${CMAKE_SOURCE_DIR}/../src/libc/stdio.h
    ${CMAKE_SOURCE_DIR}/../src/libc/stdlib.h
    ${CMAKE_SOURCE_DIR}/../src/libc/string.h
    ${CMAKE_SOURCE_DIR}/../src/libc/sys/types.h
    ${CMAKE_SOURCE_DIR}/../src/libc/time.h
    ${CMAKE_SOURCE_DIR}/../src/libc/unistd.h
)
target_include_directories(test_c PUBLIC ${CMAKE_SOURCE_DIR}/../src/libc})
target_compile_options(test_c PRIVATE
    -Wall -Werror -fno-exceptions -fno-rtti -ggdb3)
target_compile_definitions(test_c PRIVATE -DTESTING_LIBC=1)
add_executable(test_harness
    main.cpp
    test_optional.cpp
    test_ring_buffer.cpp
    test_string.cpp
)
target_link_libraries(test_harness
    test_c
    GTest::gtest_main)
target_include_directories(test_harness PRIVATE ${CMAKE_SOURCE_DIR}/../src)
target_compile_definitions(test_harness PRIVATE -DTESTING_LIBC=1)
target_compile_options(test_harness PRIVATE
    -Wall -Werror -Wno-unused-const-variable -fno-rtti -fno-exceptions -ggdb3)
gtest_discover_tests(test_harness)
