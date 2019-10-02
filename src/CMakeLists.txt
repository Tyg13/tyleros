add_subdirectory(boot)
add_subdirectory(kernel)

add_library(crti OBJECT crti.asm)
add_library(crtn OBJECT crtn.asm)

add_library(start OBJECT start.asm)

execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -print-file-name=crtbegin.o OUTPUT_VARIABLE CRTBEGIN_OBJ OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -print-file-name=crtend.o   OUTPUT_VARIABLE CRTEND_OBJ   OUTPUT_STRIP_TRAILING_WHITESPACE)

add_executable(kernel.elf
    $<TARGET_OBJECTS:crti>
    ${CRTBEGIN_OBJ}
    $<TARGET_OBJECTS:kernel>
    $<TARGET_OBJECTS:start>
    $<TARGET_OBJECTS:crtn>
    ${CRTEND_OBJ})

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/kernel.ld")

target_link_options  (kernel.elf PRIVATE -T${LINKER_SCRIPT} -ffreestanding -n -g)
set_target_properties(kernel.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
target_link_libraries(kernel.elf PRIVATE -nostdlib -lgcc)

add_custom_target(boot.bin ALL
    COMMAND objcopy -O binary $<TARGET_FILE:boot.elf> boot.bin
    DEPENDS boot.elf
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_custom_target(boot.img ALL
    COMMAND dd status=none if=/dev/zero                 of=boot.img bs=512 count=2880
    COMMAND dd status=none if=boot.bin                  of=boot.img bs=512
    COMMAND dd status=none if=$<TARGET_FILE:kernel.elf> of=boot.img bs=512 seek=3
    DEPENDS boot.bin kernel.elf
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
