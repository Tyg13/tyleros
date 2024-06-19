add_subdirectory(boot)
add_subdirectory(libc)
add_subdirectory(kernel)

# 10 sectors (5K) seems like plenty of room for now
# TODO figure out bug when trying to bump sectors up to 0x10
set(NUM_STAGE2_SECTORS 10)
math(EXPR NUM_RESERVED_SECTORS "${NUM_STAGE2_SECTORS} + 1")

add_custom_target(boot.img ALL
    COMMAND dd status=none if=/dev/zero  of=boot.img bs=512 count=2880
    COMMAND dd status=none if=stage1.bin of=boot.img bs=512 count=1 conv=notrunc
    COMMAND mformat -i boot.img -f1440 -k -R "${NUM_RESERVED_SECTORS}" ::/
    COMMAND dd status=none if=stage2.bin of=boot.img bs=512 seek=1
	count="${NUM_STAGE2_SECTORS}" conv=notrunc
    COMMAND mcopy   -i boot.img kernel.bin ::/kernel
    DEPENDS stage1.bin stage2.bin kernel.bin
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

get_target_property(LIBK_SOURCE_DIR k SOURCE_DIR)
add_custom_target(sysroot ALL
    COMMAND mkdir -p ${CMAKE_SOURCE_DIR}/sysroot/usr/include ${CMAKE_SOURCE_DIR}/sysroot/usr/lib
    COMMAND find ${LIBK_SOURCE_DIR}
	-type f -name '*.h' -exec cp '{}' "${CMAKE_SOURCE_DIR}/sysroot/usr/include/" '\;'
    COMMAND find $<TARGET_FILE_DIR:k>
	-type f -name '*.a' -exec cp '{}' ${CMAKE_SOURCE_DIR}/sysroot/usr/lib/ '\;'
    DEPENDS k
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
