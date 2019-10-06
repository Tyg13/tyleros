[BITS 64]

; Assumed that rsi points to the start
; of a valid ELF binary
global load_elf_binary
load_elf_binary:
    ; Check for magic number
    mov eax, [rsi]
    mov ebx, [ELF_MAGIC] ; 0x7F ELF
    cmp eax, ebx
    jne .end

    mov r11, rsi        ; Store start of binary
.load_program_headers:
    mov r10, [rsi + 32] ; Offset for start of program table
    mov dx,  [rsi + 56] ; Number of entries
.load_program_header:
    add r10, r11
    mov ebx, [r10]      ; Segment type
    cmp ebx, 1          ; Load segment
    jne .skip_segment

    mov r8, [r10 + 8]
    mov [p_offset], r8

    mov r8, [r10 + 16]
    mov [p_vaddr], r8
    
    mov r8, [r10 + 32]
    mov [p_filesz], r8

    mov r8, [r10 + 40]
    mov [p_memsz], r8

    ; Clear block
    xor rax, rax
    mov rdi, [p_vaddr]
    mov rcx, [p_memsz]
    rep stosb

    ; Load p_filesz bytes from file_start + p_offset
    ; to p_vaddr
    mov r12, r11
    add r12, [p_offset]
    mov rsi, r12
    mov rdi, [p_vaddr]
    mov rcx, [p_filesz]
    rep movsb

.skip_segment:
    add r10, 8

    dec dx
    jnz .load_program_header

.end:
    ; Return address for bootloader to jump to
    mov rdi, [p_vaddr]
    ret

p_offset: dq 0
p_vaddr: dq 0

p_filesz: dq 0
p_memsz: dq 0

ELF_MAGIC: db 0x7F, 'E', 'L', 'F'
