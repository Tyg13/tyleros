execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -print-file-name=crtbegin.o OUTPUT_VARIABLE CRTBEGIN_OBJ OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -print-file-name=crtend.o   OUTPUT_VARIABLE CRTEND_OBJ   OUTPUT_STRIP_TRAILING_WHITESPACE)

add_library(crti.o  OBJECT crti.asm)
add_library(crtn.o  OBJECT crtn.asm)
add_library(start.o OBJECT start.asm)
add_library(sse.o   OBJECT sse.asm)

add_executable(kernel.debug.elf
    $<TARGET_OBJECTS:crti.o>
    $<TARGET_OBJECTS:crtn.o>
    $<TARGET_OBJECTS:start.o>
    $<TARGET_OBJECTS:sse.o>
    ${CRTEND_OBJ}
    ${CRTBEGIN_OBJ}

    cmos.cpp
    gdt.cpp
    idt.cpp
    interrupts.cpp
    kernel.cpp
    memory.cpp
    paging.cpp
    pic.cpp
    pit.cpp
    pma.cpp
    vga.cpp
    vma.cpp
    util/new.cpp)

set_source_files_properties(interrupts.cpp PROPERTIES COMPILE_OPTIONS -mgeneral-regs-only)

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/kernel.ld")
target_link_options       (kernel.debug.elf PRIVATE -T${LINKER_SCRIPT} -n -L$<TARGET_FILE_DIR:libk>)
set_target_properties     (kernel.debug.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
target_link_libraries     (kernel.debug.elf PRIVATE -nostdlib -lgcc libk)
target_compile_options    (kernel.debug.elf PRIVATE ${COMMON_COMPILE_OPTIONS})
get_target_property(LIBK_SOURCE_DIR libk SOURCE_DIR)
target_include_directories(kernel.debug.elf PRIVATE SYSTEM ${LIBK_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} util)

add_custom_target(kernel.elf ALL
    COMMAND strip -g $<TARGET_FILE:kernel.debug.elf> -o kernel.elf
    DEPENDS kernel.debug.elf
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
