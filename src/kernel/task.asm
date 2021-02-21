[BITS 64]
extern frame_handler

extern should_task_switch
extern ticks_since_boot

global scheduler_interrupt
scheduler_interrupt:
    inc qword [ticks_since_boot]
    cmp byte [should_task_switch], 1
    je task_switch
end_of_interrupt:
    push rax

    ; Signal end of CMOS RTC interrupt
    mov al, 0xC
    out 0x70, al
    in  al, 0x71

    ; Signal end of IRQ to slave and master PIC
    ; (since RTC interrupt is 0x28, which is a slave IRQ)
    mov al, 0x20
    out 0xA0, al
    out 0x20, al

    pop rax
    iretq

task_switch:
    ; Push all registers that weren't already pushed by the processor
    ; when the interrupt fired
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rdi, rsp
    call frame_handler
    ; A new frame (if we switched frames) is returned on the stack
    ; Restore the context
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    jmp end_of_interrupt
