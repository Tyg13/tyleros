execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -print-file-name=crtbegin.o OUTPUT_VARIABLE CRTBEGIN_OBJ OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -print-file-name=crtend.o   OUTPUT_VARIABLE CRTEND_OBJ   OUTPUT_STRIP_TRAILING_WHITESPACE)

add_library(asm.o OBJECT
   crti.asm
   crtn.asm
   start.asm
   task.asm)
target_compile_options (asm.o PRIVATE -g -F dwarf)

add_executable(kernel.elf
    $<TARGET_OBJECTS:asm.o>
    ${CRTEND_OBJ}
    ${CRTBEGIN_OBJ}

    alloc.cpp
    crc32.c
    cmos.cpp
    debug.cpp
    dma.cpp
    elf.cpp
    gdt.cpp
    filesystem.cpp
    floppy.cpp
    idt.cpp
    interrupts.cpp
    input.cpp
    low_memory_allocator.cpp
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
    vma.cpp)

# Can only use certain instructions in interrupt handlers
set_source_files_properties(interrupts.cpp PROPERTIES COMPILE_OPTIONS -mgeneral-regs-only)
set_source_files_properties(scheduler.cpp PROPERTIES COMPILE_OPTIONS -mgeneral-regs-only)

get_target_property(LIBK_SOURCE_DIR c SOURCE_DIR)
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/kernel.ld")
target_link_options       (kernel.elf PRIVATE "-T${LINKER_SCRIPT}" "-L$<TARGET_FILE_DIR:c>")
set_target_properties     (kernel.elf PROPERTIES LINK_DEPENDS "${LINKER_SCRIPT}")
target_link_libraries     (kernel.elf PRIVATE -nostdlib k)
target_include_directories(kernel.elf PRIVATE
    "${LIBK_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    util
    adt
)

add_custom_target(generate_kernel.bin
    COMMAND "${CMAKE_STRIP}" --strip-debug "$<TARGET_FILE:kernel.elf>" -o kernel.bin
    DEPENDS kernel.elf
    BYPRODUCTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.bin
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_custom_target(generate_kernel.sym
    COMMAND "${CMAKE_STRIP}" --only-keep-debug "$<TARGET_FILE:kernel.elf>" -o kernel.sym
    DEPENDS kernel.elf
    BYPRODUCTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.sym
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
