add_subdirectory(boot)
add_subdirectory(libc)
add_subdirectory(kernel)

add_custom_target(boot.bin ALL
    COMMAND objcopy -O binary $<TARGET_FILE:boot.elf> boot.bin
    DEPENDS boot.elf
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_custom_target(boot.img ALL
    COMMAND dd status=none if=/dev/zero  of=boot.img bs=512 count=2880
    COMMAND dd status=none if=boot.bin   of=boot.img bs=512
    COMMAND dd status=none if=kernel.elf of=boot.img bs=512 seek=2
    DEPENDS boot.bin kernel.elf
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
