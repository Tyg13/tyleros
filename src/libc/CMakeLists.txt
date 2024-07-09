set(LIBC_SOURCES
    assert.cpp
    errno.cpp
    stdio.cpp
    stdlib.cpp
    string.cpp
    time.cpp
    unistd.cpp)

add_library(c STATIC ${LIBC_SOURCES})
add_library(k STATIC ${LIBC_SOURCES})

target_include_directories(c PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(k PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(k PRIVATE -DIN_KERNEL=1)
