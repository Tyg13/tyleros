set(CMAKE_ASM_NASM_LINK_FLAGS "-N")

macro(add_stage_linker_opts stage)
    target_link_options(${stage}.elf PRIVATE "-T${CMAKE_CURRENT_SOURCE_DIR}/${stage}.ld")
    set_target_properties(${stage}.elf PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${stage}.ld")
endmacro()

add_executable(stage1.elf stage1.asm utils.asm)
target_include_directories(stage1.elf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_stage_linker_opts(stage1)
add_custom_target(generate_stage1.bin
    COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:stage1.elf>" stage1.bin
    DEPENDS stage1.elf
    BYPRODUCTS stage1.bin
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_executable(stage2.elf
    a20.asm
    allocator.asm
    elf_loader.asm
    fat.asm
    memory_map.asm
    paging.asm
    printing.asm
    read_floppy.asm
    stage2.asm
    utils.asm)
target_include_directories(stage2.elf PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
add_stage_linker_opts(stage2)
add_custom_target(generate_stage2.bin
    COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:stage2.elf>" stage2.bin
    DEPENDS stage2.elf
    BYPRODUCTS stage2.bin
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_custom_command(OUTPUT compute_crc32
    COMMAND ${HOST_C_COMPILER} -o compute_crc32
      ${CMAKE_CURRENT_SOURCE_DIR}/compute_crc32.c
      ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/crc32.c 
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/compute_crc32
    DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/crc32.c 
      ${CMAKE_CURRENT_SOURCE_DIR}/compute_crc32.c)
add_custom_target(generate_kernel.crc32
    COMMAND ./compute_crc32
      ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.bin
      ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.crc32
    BYPRODUCTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.crc32
    DEPENDS generate_kernel.bin ${CMAKE_CURRENT_BINARY_DIR}/compute_crc32)
set_source_files_properties(stage2.asm OBJECT_DEPENDS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.crc32)
