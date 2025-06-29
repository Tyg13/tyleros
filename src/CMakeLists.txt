add_subdirectory(boot)
add_subdirectory(libc)
add_subdirectory(libadt)

get_target_property(LIBK_SOURCE_DIR k SOURCE_DIR)
add_custom_target(sysroot
    COMMAND mkdir -p ${CMAKE_SOURCE_DIR}/sysroot/usr/include ${CMAKE_SOURCE_DIR}/sysroot/usr/lib
    COMMAND find ${LIBK_SOURCE_DIR}
	-type f -name '*.h' -exec cp '{}' "${CMAKE_SOURCE_DIR}/sysroot/usr/include/" '\;'
    COMMAND find $<TARGET_FILE_DIR:c>
	-type f -name '*.a' -exec cp '{}' ${CMAKE_SOURCE_DIR}/sysroot/usr/lib/ '\;'
    DEPENDS c
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_subdirectory(kernel)
add_dependencies(kernel.elf sysroot)

find_package(Python COMPONENTS Interpreter)
add_custom_command(
    OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/boot.img
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/boot/make_boot_img.py
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/stage1.bin
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/stage2.bin
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.bin
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.sym
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    DEPENDS
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/stage1.bin
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/stage2.bin
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.bin
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kernel.sym
)
add_custom_target(final_img ALL
    DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/boot.img)
