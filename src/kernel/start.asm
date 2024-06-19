[BITS 64]

extern _init, _fini, kmain

global _start
_start:
    ; Enable SSE for global ctors called in _init
    mov rax, cr0
    ; Disable FPU emulation
    ; Enable WAIT/FWAIT
    and ax, ~CR0.EM
    or ax, CR0.MP
    mov cr0, rax
    ; Enable SSE and FPU save/restore
    ; Enable unmasked SSE exceptions
    mov rax, cr4
    or ax, CR4.OSFXSR | CR4.OSXMMEXCPT
    mov cr4, rax

    push rdi
    call _init
    pop rdi

    call kmain

    call _fini

    cli
    hlt
    jmp $

CR0.EM equ 1 << 2
CR0.MP equ 1 << 1
CR4.OSFXSR equ 1 << 9
CR4.OSXMMEXCPT equ 1 << 10
