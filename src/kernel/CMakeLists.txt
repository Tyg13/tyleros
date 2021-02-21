execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -print-file-name=crtbegin.o OUTPUT_VARIABLE CRTBEGIN_OBJ OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -print-file-name=crtend.o   OUTPUT_VARIABLE CRTEND_OBJ   OUTPUT_STRIP_TRAILING_WHITESPACE)

add_library(asm.o OBJECT
   crti.asm
   crtn.asm
   start.asm
   sse.asm
   task.asm)
target_compile_options (asm.o PRIVATE -g -F dwarf)

add_executable(kernel.debug.elf
    $<TARGET_OBJECTS:asm.o>
    ${CRTEND_OBJ}
    ${CRTBEGIN_OBJ}

    cmos.cpp
    debug.cpp
    dma.cpp
    gdt.cpp
    filesystem.cpp
    floppy.cpp
    idt.cpp
    interrupts.cpp
    main.cpp
    memory.cpp
    paging.cpp
    pic.cpp
    pit.cpp
    pma.cpp
    scheduler.cpp
    serial.cpp
    timing.cpp
    vga.cpp
    vma.cpp
    util/new.cpp)

# Can only use certain instructions in interrupt handlers
set_source_files_properties(interrupts.cpp PROPERTIES COMPILE_OPTIONS -mgeneral-regs-only)

get_target_property(LIBK_SOURCE_DIR k SOURCE_DIR)
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/kernel.ld")
target_link_options       (kernel.debug.elf PRIVATE -T${LINKER_SCRIPT} -n -L$<TARGET_FILE_DIR:k>)
set_target_properties     (kernel.debug.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
target_link_libraries     (kernel.debug.elf PRIVATE -nostdlib -lgcc -lk)
target_include_directories(kernel.debug.elf PRIVATE SYSTEM ${LIBK_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} util)

add_custom_target(kernel.elf ALL
    COMMAND strip -g $<TARGET_FILE:kernel.debug.elf> -o kernel.elf
    DEPENDS kernel.debug.elf
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
