[BITS 64]

extern _init, _fini, kmain

global _start
_start:
    push rdi
    call _init
    pop rdi

    call kmain

    call _fini

    cli
    hlt
    jmp $
