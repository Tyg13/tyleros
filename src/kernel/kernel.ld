ENTRY(_start)

KERNEL_VMA_BASE = 0xC000000;
KERNEL_LMA_BASE = 0x100000;

SECTIONS
{
    . = KERNEL_VMA_BASE;
    __KERNEL_VMA_START__ = .;
    __KERNEL_LMA_START__ = . - (KERNEL_VMA_BASE - KERNEL_LMA_BASE);
    .start : AT(ADDR(.start) - KERNEL_VMA_BASE + KERNEL_LMA_BASE)
    {
        *start.*(.text)
    }
    .init :
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
    __KERNEL_VMA_END__ = .;
    __KERNEL_LMA_END__ = . - (KERNEL_VMA_BASE - KERNEL_LMA_BASE);

    /DISCARD/ :
    {
        *(.comment)
        *(.note)
    }
}
