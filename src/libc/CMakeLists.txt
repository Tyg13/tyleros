add_library(k STATIC
    errno.cpp
    stdio.cpp
    stdlib.cpp
    string.cpp
    time.cpp
    unistd.cpp)

target_include_directories(k PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
