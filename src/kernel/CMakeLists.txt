execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -print-file-name=crtbegin.o OUTPUT_VARIABLE CRTBEGIN_OBJ OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -print-file-name=crtend.o   OUTPUT_VARIABLE CRTEND_OBJ   OUTPUT_STRIP_TRAILING_WHITESPACE)

add_library(crti.o  OBJECT crti.asm)
add_library(crtn.o  OBJECT crtn.asm)
add_library(start.o OBJECT start.asm)

add_executable(kernel.elf
    $<TARGET_OBJECTS:crti.o>
    $<TARGET_OBJECTS:crtn.o>
    $<TARGET_OBJECTS:start.o>
    ${CRTEND_OBJ}
    ${CRTBEGIN_OBJ}

    kernel.cpp
    idt.cpp
    interrupts.cpp)

set_source_files_properties(interrupts.cpp PROPERTIES COMPILE_OPTIONS -mgeneral-regs-only)

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/kernel.ld")
target_link_options   (kernel.elf PRIVATE -T${LINKER_SCRIPT} -ffreestanding -n -g)
set_target_properties (kernel.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
target_link_libraries (kernel.elf PRIVATE -nostdlib -lgcc)
target_compile_options(kernel.elf PRIVATE -Wall -Werror -mno-red-zone -ffreestanding -fno-exceptions -fno-rtti -g)
