[BITS 16]

boot:
    xor eax, eax
    mov es, eax
    mov di, PAGE_TABLE
    mov ecx, 0x1000
    cld
    rep stosd

    mov di, PAGE_TABLE
    ; Page Map Level 4
    lea eax, [es:di + 0x1000]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [es:di], eax

    ; Page Directory Pointer Table
    lea eax, [es:di + 0x2000]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [es:di + 0x1000], eax

    ; Page Directory
    lea eax, [es:di + 0x3000]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [es:di + 0x2000], eax

    ; Page Table
    lea di, [di + 0x3000]
    mov eax, PAGE_PRESENT | PAGE_PRESENT
.build_page_entry:
    mov [es:di], eax
    add eax, 0x1000
    add di, 0x8
    cmp eax, 0x200000
    jb .build_page_entry

    ; Disable IRQs
    mov al, 0xFF
    out 0xA1, al
    out 0x21, al
    nop
    nop

    lidt [idt]

    mov eax, CR4_PAE | CR4_PGE
    mov cr4, eax

    mov edx, PAGE_TABLE
    mov cr3, edx

    mov ecx, 0xC0000080
    rdmsr

    or eax, LME
    wrmsr

    mov ebx, cr0
    or ebx, CR0_PAGE | CR0_PROT
    mov cr0, ebx

    lgdt [gdt.descriptor]

    jmp CODE_SEG:long_mode

ALIGN 4
idt:
    .length dw 0x0
    .base   dd 0x0

gdt:
.null:
    dq 0x0
.code:
    dq 0x00209A0000000000
    dq 0x00009A0000000000
ALIGN 4
    dw 0
.descriptor:
    .size    dw $ - gdt - 1
    .address dd gdt

[BITS 64]
long_mode:
    jmp $

times 510 - ($ - $$) db 0 ; Boot sector is 512 bytes
dw 0xAA55 ; Boot signature

CODE_SEG equ 0x8
DATA_SEG equ 0x10
PAGE_TABLE equ 0xA000
PAGE_PRESENT equ (1 << 0)
PAGE_WRITE   equ (1 << 1)
CR0_PAGE equ (1 << 31)
CR0_PROT equ (1 << 0)
CR4_PAE equ (1 << 5)
CR4_PGE equ (1 << 7)
LME equ 0x100
