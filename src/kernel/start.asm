[BITS 64]

extern _init, _fini, kmain
extern __bss_start__, __bss_end__

boot_info: dq 0

global _start
_start:
    mov qword [boot_info], rdi

    ; Zero out bss segment in anticipation of global ctors running
    lea rdi, [__bss_start__]
    lea rcx, [__bss_end__]
    sub rcx, rdi
    xor al, al
    rep stosb

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

    and rsp, -16 ; 16-byte align the stack if it wasn't already
    call _init

    mov rdi, qword [boot_info]
    call kmain

    call _fini

    cli
    hlt
    jmp $

CR0.EM equ 1 << 2
CR0.MP equ 1 << 1
CR4.OSFXSR equ 1 << 9
CR4.OSXMMEXCPT equ 1 << 10
