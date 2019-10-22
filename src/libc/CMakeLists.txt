add_library(libk STATIC
    errno.cpp
    stdio.cpp
    stdlib.cpp
    string.cpp
    time.cpp
    unistd.cpp)

target_include_directories(libk PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options    (libk PRIVATE ${COMMON_COMPILE_OPTIONS})
