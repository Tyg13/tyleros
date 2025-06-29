set(LIBC_SOURCES
    assert.cpp
    ctype.cpp
    errno.cpp
    stdio.cpp
    stdlib.cpp
    string.cpp
    time.cpp
    unistd.cpp
)
set(LIBC_HEADERS
    assert.h
    ctype.h
    errno.h
    stdio.h
    stdlib.h
    string.h
    sys/types.h
    time.h
    unistd.h
)

add_library(c STATIC ${LIBC_SOURCES} ${LIBC_HEADERS})
add_library(k STATIC ${LIBC_SOURCES} ${LIBC_HEADERS})

target_include_directories(c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(k PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(k PUBLIC -DLIBC_IN_KERNEL=1)

target_compile_options(k PRIVATE
    -Wall -Werror -mno-red-zone -ffreestanding -fno-exceptions -fno-rtti -mcmodel=kernel -fno-pic -ggdb3 -mgeneral-regs-only -O2)
target_compile_options(c PRIVATE
    -Wall -Werror -mno-red-zone -ffreestanding -fno-exceptions -fno-rtti -mcmodel=kernel -fno-pic -ggdb3)
