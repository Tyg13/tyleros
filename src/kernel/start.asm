[BITS 64]

extern _init, _fini, kmain

global _start
_start:
    call _init

    call kmain

    call _fini

    cli
    hlt
    jmp $
