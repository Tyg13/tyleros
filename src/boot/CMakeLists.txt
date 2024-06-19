set(CMAKE_ASM_NASM_LINK_FLAGS "-N")

macro(add_stage_linker_opts stage)
    target_link_options(${stage}.elf PRIVATE "-T${CMAKE_CURRENT_SOURCE_DIR}/${stage}.ld")
    set_target_properties(${stage}.elf PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${stage}.ld")
endmacro()

add_executable(stage1.elf stage1.asm utils.asm)
target_include_directories(stage1.elf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_stage_linker_opts(stage1)
add_custom_target(stage1.bin ALL
    COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:stage1.elf>" stage1.bin
    DEPENDS stage1.elf
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_executable(stage2.elf
    allocator.asm
    elf_loader.asm
    fat.asm
    memory_map.asm
    paging.asm
    printing.asm
    read_floppy.asm
    stage2.asm
    utils.asm)
target_include_directories(stage2.elf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_stage_linker_opts(stage2)
add_custom_target(stage2.bin ALL
    COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:stage2.elf>" stage2.bin
    DEPENDS stage2.elf
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
