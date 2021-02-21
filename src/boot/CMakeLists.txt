set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/boot.ld")
set(CMAKE_ASM_NASM_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/")
set(CMAKE_ASM_NASM_LINK_FLAGS "-T${LINKER_SCRIPT} -N")

add_executable(boot.elf boot1.asm boot2.asm)
add_custom_target(elf_loader DEPENDS elf_loader.asm)
add_dependencies(boot.elf elf_loader)
set_target_properties(boot.elf PROPERTIES LINK_DEPENDS "${LINKER_SCRIPT}")
