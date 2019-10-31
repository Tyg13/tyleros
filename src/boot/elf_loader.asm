[BITS 64]

; Assumed that rsi points to the start
; of a valid ELF binary
load_elf_binary:
    ; Check for magic number
    mov eax, [rsi]
    cmp eax, ELF_MAGIC
    jne .end

    mov r8, rsi         ; Store start of binary
.load_program_headers:
    mov r9, [rsi + 32] ; Get offset for start of program table
    mov dx, [rsi + 56] ; Number of entries
    add r9, r8         ; r9 tracks the current progam header's start
.load_program_header:
    mov ebx, [r9]      ; Get segment type type
    cmp ebx, 1         ; 1 means load segment -- ignore all others
    jne .skip_segment

    mov r11, [r9 + 8]  ; physical offset
    mov r12, [r9 + 16] ; virtual address
    mov r13, [r9 + 32] ; physical file size
    mov r14, [r9 + 40] ; memory size

    ; Clear block
    xor rax, rax
    mov rdi, r12
    mov rcx, r14
    rep stosb

    ; Load p_filesz bytes from file_start + p_offset
    ; to p_vaddr
    mov r15, r8
    add r15, r11
    mov rsi, r15
    mov rdi, r12
    mov rcx, r13
    rep movsb

.skip_segment:
    add r10, 8

    dec dx
    jnz .load_program_header

.end:
    ; Return address for bootloader to jump to
    mov rdi, [r8 + 0x18] ; Program entry position
    ret

ELF_MAGIC equ `\x7fELF`
