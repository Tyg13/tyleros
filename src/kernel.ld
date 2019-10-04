ENTRY(_start)

SECTIONS
{
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
