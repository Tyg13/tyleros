ENTRY(_start)

SECTIONS
{
    /* Keep in sync with boot 2 */
    . = 0xC000000;
    .start :
    {
        *start.*(.text)
    }
    .init : {
        *crti*(.init)
        *crtbegin*(.init)
        *crtend*(.init)
        *crtn*(.init)
    }
    .fini : {
        *crti*(.fini)
        *crtbegin*(.fini)
        *crtend*(.fini)
        *crtn*(.fini)
    }
}
