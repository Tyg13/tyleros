SECTIONS {
    . = 0x700;
    .text : {
        *stage2.asm.obj (.text) /* make sure _start is linked first */
        *(.text)
    }
    .data : {
        . = ALIGN(4);
        *(.data)
    }
}
