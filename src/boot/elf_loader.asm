[BITS 64]

; Assumed that rsi points to the start
; of a valid ELF binary
global load_elf_binary
load_elf_binary:
    ; Check for magic number
    mov eax, [rsi]
    cmp eax, ELF_MAGIC
    jne .error

    push rsi            ; Store start of binary
.load_program_headers:
    mov rbp, rsi
    add rbp, [rsi + 32] ; Get offset for start of program table
                        ; rbp tracks the current program header
    movzx r8,  word [rsi + 54]  ; Size of a program header
    movzx rdx, word [rsi + 56]  ; Number of program headers
.load_program_header:
    mov ax, word [rbp]  ; Get segment type
    cmp ax, 1           ; 1 means load segment -- ignore all others
    jne .skip_segment

    mov r11, qword [rbp + 8]  ; physical offset
    mov r12, qword [rbp + 16] ; virtual address
    mov r13, qword [rbp + 32] ; physical file size
    mov r14, qword [rbp + 40] ; memory size

    ; Clear block
    xor rax, rax
    mov rdi, r12
    mov rcx, r14
    rep stosb

    ; Load p_filesz bytes from file_start + p_offset
    ; to p_vaddr
    mov r15, qword [rsp] ; load file start
    add r15, r11
    mov rsi, r15
    mov rdi, r12
    mov rcx, r13
    rep movsb

.skip_segment:
    add rbp, r8

    dec dx
    jnz .load_program_header

.end:
    pop rsi
    ; Return address for bootloader to jump to
    mov rax, [rsi + 0x18] ; Program entry position
    ret

.error:
    hlt
    jmp .error

ELF_MAGIC equ `\x7fELF`
