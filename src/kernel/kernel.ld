ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)

KERNEL_VMA_BASE = 0xFFFFFFFF80000000;
KERNEL_LMA_BASE = 0x100000;

SECTIONS
{
     . = KERNEL_VMA_BASE;
    __text_start__ = .;
    .init : AT(KERNEL_LMA_BASE)
    {
        *crti*(.init)
        *crtbegin*(.init)
        *crtend*(.init)
        *crtn*(.init)
    }
    .fini :
    {
        *crti*(.fini)
        *crtbegin*(.fini)
        *crtend*(.fini)
        *crtn*(.fini)
    }
    .ctors :
    {
        *crtbegin*(.ctors)
        *(EXCLUDE_FILE (*crtend*) .ctors)
        *crtend*(.ctors)
    }
    .dtors :
    {
        *crtbegin*(.dtors)
        *(EXCLUDE_FILE (*crtend*) .dtors)
        *crtend*(.dtors)
    }
    .text : ALIGN(4K) { *(.text*) *(.eh_frame*) *(.tm_clone_table) }
    . = ALIGN(4K);
    __text_end__ = .;
    __rodata_start__ = .;
    .rodata : { *(.rodata*) }
    . = ALIGN(4K);
    __rodata_end__ = .;
    __data_start__ = .;
    .data : { *(.data*) }
    . = ALIGN(4K);
    __data_end__ = .;
    __bss_start__ = .;
    .bss : { *(.bss COMMON) }
    . = ALIGN(4K);
    __bss_end__ = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.note)
    }
}
