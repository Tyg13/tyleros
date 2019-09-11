add_library(boot1.o OBJECT boot1.asm)
add_library(boot2.o OBJECT boot2.asm)
add_custom_target(boot.img ALL
    COMMAND dd if=$<TARGET_OBJECTS:boot1.o> of=boot.img bs=512 count=1
    COMMAND dd if=$<TARGET_OBJECTS:boot2.o> of=boot.img bs=512 count=1    seek=1
    COMMAND dd if=/dev/zero                 of=boot.img bs=512 count=2778 seek=2
    DEPENDS boot1.o boot2.o
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
