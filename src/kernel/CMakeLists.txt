add_library(kernel OBJECT
    kernel.cpp
    idt.cpp
)
set_source_files_properties(idt.cpp PROPERTIES COMPILE_OPTIONS -mgeneral-regs-only)
target_include_directories(kernel PRIVATE "${PROJECT_SOURCE_DIR}/inc/kernel")
target_compile_options    (kernel PRIVATE -Wall -Werror -mno-red-zone -ffunction-sections -ffreestanding -fno-exceptions -fno-rtti -g)
