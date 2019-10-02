[BITS 16]

extern load_elf_binary

boot:
    ; Clear Page Map Level 4
    xor eax, eax
    mov es, eax
    mov di, PAGE_TABLE
    mov ecx, 0x1000
    cld
    rep stosd
; Identity map
    ; 0x0 - 0x200000
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

    mov eax, 0x1FFFF0
    mov esp, eax
    mov ebp, esp

    ;Map kernel pages
    ;0xC000000 - 0xC200000 to 0x100000 - 0x300000
    mov di, PAGE_TABLE
    ; Page Directory Entry
    lea eax, [es:di + 0x4000]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [es:di + 0x2000 + 8 * 0x60], eax

    ; Page Table Entry
    lea di, [di + 0x4000]
    mov eax, 0x100000
    or eax, PAGE_PRESENT | PAGE_PRESENT
.build_kernel_page_entry:
    mov [es:di], eax
    add eax, 0x1000
    add di, 0x8
    cmp eax, 0x300000
    jb .build_kernel_page_entry

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
.data:
    dq 0x0000920000000000
ALIGN 4
    dw 0
.descriptor:
    .size    dw $ - gdt - 1 
    .address dd gdt

[BITS 64]
long_mode:
    mov ax, DATA_SEG
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rsi, 0x8000
    call load_elf_binary

    ; Keep this address in sync with kernel.ld
    mov rbx, 0xC000000
    jmp rbx

times 510 - ($ - $$) db 0 ; Boot sector is 512 bytes
dw 0xAA55 ; Boot signature

CODE_SEG equ gdt.code - gdt
DATA_SEG equ gdt.data - gdt
PAGE_TABLE equ 0xA000
PAGE_PRESENT equ (1 << 0)
PAGE_WRITE   equ (1 << 1)
CR0_PAGE equ (1 << 31)
CR0_PROT equ (1 << 0)
CR4_PAE equ (1 << 5) 
CR4_PGE equ (1 << 7)
LME equ 0x100
