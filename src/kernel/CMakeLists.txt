execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -print-file-name=crtbegin.o OUTPUT_VARIABLE CRTBEGIN_OBJ OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -print-file-name=crtend.o   OUTPUT_VARIABLE CRTEND_OBJ   OUTPUT_STRIP_TRAILING_WHITESPACE)

add_library(asm.o OBJECT
   crti.asm
   crtn.asm
   start.asm
   syscall.asm
   task.asm)
target_compile_options (asm.o PRIVATE -g -F dwarf)

set(KERNEL_SOURCES
    acpi.cpp
    alloc.cpp
    atexit.cpp
    crc32.cpp
    cmos.cpp
    debug.cpp
    dma.cpp
    dwarf.cpp
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
    minishell.cpp
    paging.cpp
    panic.cpp
    pic.cpp
    pit.cpp
    pma.cpp
    scheduler.cpp
    serial.cpp
    syscalls.cpp
    timing.cpp
    vga.cpp
    vma.cpp
)
add_executable(kernel.elf
    $<TARGET_OBJECTS:asm.o>
    ${CRTEND_OBJ}
    ${CRTBEGIN_OBJ}

    ${KERNEL_SOURCES}
)

# Can only use certain instructions in interrupt handlers
set_source_files_properties(interrupts.cpp PROPERTIES COMPILE_OPTIONS -mgeneral-regs-only)
set_source_files_properties(scheduler.cpp PROPERTIES COMPILE_OPTIONS -mgeneral-regs-only)

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/kernel.ld")
target_compile_options    (kernel.elf PRIVATE
    -Wall -Werror -mno-red-zone -ffreestanding -fno-exceptions -fno-rtti -mcmodel=kernel -fno-pic -ggdb3)
target_link_options       (kernel.elf PRIVATE "-T${LINKER_SCRIPT}")
set_target_properties     (kernel.elf PROPERTIES LINK_DEPENDS "${LINKER_SCRIPT}")
target_link_libraries     (kernel.elf PRIVATE -nostdlib k adt)
target_include_directories(kernel.elf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_command(
    OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.bin
    COMMAND "${CMAKE_STRIP}" --strip-debug "$<TARGET_FILE:kernel.elf>" -o kernel.bin
    DEPENDS kernel.elf
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
add_custom_target(generate_kernel.bin ALL
    DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.bin)

add_custom_command(
    OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.sym
    COMMAND "${CMAKE_STRIP}" --only-keep-debug "$<TARGET_FILE:kernel.elf>" -o kernel.sym
    DEPENDS kernel.elf
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
add_custom_target(generate_kernel.sym ALL
    DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.sym)
