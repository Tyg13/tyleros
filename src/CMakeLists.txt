add_subdirectory(boot)
add_subdirectory(libc)
add_subdirectory(kernel)

add_custom_target(boot.bin ALL
    COMMAND objcopy -O binary $<TARGET_FILE:boot.elf> boot.bin
    DEPENDS boot.elf
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_custom_target(boot2.bin ALL
    COMMAND dd status=none if=boot.bin of=boot2.bin bs=512 skip=1 count=1
    DEPENDS boot.bin
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_custom_target(boot.img ALL
    COMMAND dd status=none if=/dev/zero  of=boot.img bs=512 count=2880
    COMMAND dd status=none if=boot.bin   of=boot.img bs=512 count=1
    COMMAND mformat -i boot.img -f1440 -k  ::/
    COMMAND mcopy   -i boot.img boot2.bin  ::/boot
    COMMAND mcopy   -i boot.img kernel.elf ::/kernel
    DEPENDS boot.bin boot2.bin kernel.elf
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
