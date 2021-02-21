; Stage 2 bootloader
; This stage sets up page tables, with the following mappings:
;   Physical					 Virtual                  
;   0000 0000 0000 0000 - 0000 0000 0010 0000 -> 0000 0000 0000 0000 - 0000 0000 0010 0000
;   0000 0000 0010 0000 - 0000 0000 0030 0000 -> 0000 0000 C000 0000 - 0000 0000 C000 0000
jmp boot
%include "elf_loader.asm"

[BITS 16]

boot:
    ; ebx contains the base of where the kernel was loaded.
    ; Store it for later
    mov dword [kernel_address], ebx

    xor eax, eax
    mov es, ax

    ; Clear Page Map Level 4
    mov di, PAGE_TABLE
    mov ecx, 0x1000
    cld
    rep stosd
    
    ; Identity pages
    mov di, PAGE_TABLE   ; di = 0x8000
    ; Page Map Level 4
    lea eax, [es:di + 0x1000] ; 0x9000
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [es:di], eax

    ; Page Directory Pointer Table
    lea eax, [es:di + 0x2000] ; 0xA000
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [es:di + 0x1000], eax

    ; Page Directory Entry
    lea eax, [es:di + 0x3000] ; 0xB000
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [es:di + 0x2000], eax

    ; Page Table Entry
    lea di, [di + 0x3000]     ; 0xB000
    mov eax, PAGE_PRESENT | PAGE_WRITE
.build_page_entry:
    mov [es:di], eax
    add eax, 0x1000
    add di, 0x8
    cmp eax, 0x100000
    jb .build_page_entry

    ;Map kernel pages
    mov di, PAGE_TABLE

    ; Page Map Level 4
    lea eax, [es:di + 0x4000] ; 0xD000
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [es:di + 8 * 0x1FF], eax

    ; Page Directory Pointer Entry
    lea eax, [es:di + 0x5000] ; 0xD000
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [es:di + 0x4000 + 8 * 0x1FE], eax

    ; Page Directory Entry
    lea eax, [es:di + 0x6000] ; 0xE000
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [es:di + 0x5000], eax

    ; Page Table Entry
    lea di, [di + 0x6000]     ; 0xE000
    mov eax, 0x100000 | PAGE_PRESENT | PAGE_WRITE
.build_kernel_page_entry:
    mov [es:di], eax
    add eax, 0x1000
    add di, 0x8
    cmp eax, 0x300000
    jb .build_kernel_page_entry

    ; Mask all IRQs
    mov al, 0xFF
    out 0xA1, al
    out 0x21, al
    nop
    nop

    call build_memory_map

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

[BITS 64]
long_mode:
    mov ax, DATA_SEG
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; [rsp] contains the current kernel address
    mov rsi, [kernel_address]

    ; Set up kernel stack
    mov rsp, KERNEL_STACK

    ; load_elf_binary(rsi: current_kernel_address)
    call load_elf_binary

    ; Virtual address to jump to will be returned in rax
    jmp rax

    ; Infinite loop in case we ever return (shouldn't)
    cli
    hlt
    jmp $

[BITS 16]
build_memory_map:
    xor ebp, ebp

    mov di, MEMORY_MAP_BASE + 0x8
    xor ebx, ebx
    mov edx, 0x534D4150

.entry:
    mov eax, 0xE820
    mov ecx, 24
    int 0x15
    jc .done

    mov edx, 0x534D4150
    cmp eax, edx
    jne .done

    inc bp

    test ebx, ebx
    je .done

    add di, 24

    jmp .entry

.done:
    mov di, MEMORY_MAP_BASE;
    mov [es:di], bp
    ret

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

kernel_address: dq 0

times 510 - ($ - $$) db 0 ; Boot sector is 512 bytes
dw 0xAA55 ; Boot signature

CODE_SEG equ gdt.code - gdt
DATA_SEG equ gdt.data - gdt

MEMORY_MAP_BASE equ 0x1000
KERNEL_BASE  equ 0xFFFFFFFF80000000
KERNEL_STACK equ KERNEL_BASE + 0x200000

PAGE_TABLE equ 0x8000
PAGE_PRESENT equ (1 << 0)
PAGE_WRITE   equ (1 << 1)
PAGE_LARGE   equ (1 << 7)

CR0_PAGE equ (1 << 31)
CR0_PROT equ (1 << 0)
CR4_PAE equ (1 << 5) 
CR4_PGE equ (1 << 7)
LME equ 0x100
