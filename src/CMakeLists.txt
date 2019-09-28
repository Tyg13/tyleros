set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/boot.ld")
set(CMAKE_ASM_NASM_LINK_FLAGS "-T${LINKER_SCRIPT} -N")

add_executable(boot.elf boot1.asm boot2.asm)
set_target_properties(boot.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

add_custom_target(boot.bin ALL
    COMMAND objcopy -O binary $<TARGET_FILE:boot.elf> boot.bin
    DEPENDS boot.elf
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_custom_target(boot.img ALL
    COMMAND dd status=none if=boot.bin  of=boot.img bs=512 count=2
    COMMAND dd status=none if=/dev/zero of=boot.img bs=512 count=2778 seek=2
    DEPENDS boot.bin
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
