set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/boot.ld")
set(CMAKE_ASM_NASM_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/")
set(CMAKE_ASM_NASM_LINK_FLAGS "-T${LINKER_SCRIPT} -N")

add_executable(boot.elf boot1.asm boot2.asm)
set_target_properties(boot.elf PROPERTIES LINK_DEPENDS "${LINKER_SCRIPT}")
