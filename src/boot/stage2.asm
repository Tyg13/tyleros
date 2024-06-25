; Stage 2 bootloader
; This stage sets up page tables, builds a map of physical memory, and reads
; the kernel binary from floppy. It then enters long mode, loads the kernel at
; its intended virtual address and jumps into it.
; Page Mappings
;   Physical                 Virtual                  
;   0000 0000 - 0008 0000 -> 0000 0000 - 0008 0000
;   0010 0000 - 0030 0000 -> 0C00 0000 - 0C00 0000
[BITS 16]

; elf_loader.asm
extern load_elf_binary

; fat.asm
extern read_kernel_from_filesystem

; memory_map.asm
extern build_memory_map
extern find_first_usable_region_at_least_2mb

; allocator.asm
extern allocate_pages
extern avail_mem_start
extern avail_mem_end

; paging.asm
extern map_contiguous_pages

; printing.asm
extern print_init

global _start
_start:
    ; ax contains the address of the end of our loaded stage2 binary
    ; dl contains our drive number
    mov byte [boot_info.drive_number], dl

    ; Align `ax` to a 0x1000 (4 KiB) boundary
    and ax, 0xF000
    add ax, 0x1000

    ; Use the memory between the end of stage2 and our stack for allocation
    mov word [avail_mem_start], ax
    mov dword [avail_mem_end], LOW_MEM_END - PAGE_SIZE

    ; Zero out the last page in low memory and put the stack there
    mov ax, (LOW_MEM_END - PAGE_SIZE) / 0x10
    mov es, ax
    mov di, (LOW_MEM_END - PAGE_SIZE) % 0x10
    xor ax, ax
    mov cx, PAGE_SIZE
    rep stosb

    mov ax, (LOW_MEM_END - PAGE_SIZE) / 0x10
    mov ss, ax
    mov sp, PAGE_SIZE

    call print_init

    ; Allocate a page for the memory map
    mov cx, 1
    call allocate_pages
    mov dword [boot_info.memory_map_base], eax
    
    ; build_memory_map(memory_map_base: ax)
    call build_memory_map
    mov dword [boot_info.num_memory_map_entries], eax

    mov dl, byte [boot_info.drive_number]
    call read_kernel_from_filesystem
    mov dword [boot_info.kernel_physical_start], eax
    mov dword [boot_info.kernel_physical_end], edx
    mov dword [boot_info.kernel_boot_size], ecx
    mov dword [avail_mem_start], edx

    ; Use the rest of the available low memory for page tables
    ; aligned to a 0x1000 (4 KiB) boundary
    mov eax, edx
    and eax, 0xFFFFF000
    add eax, 0x1000
    mov dword [page_table], eax

    ; Identity map the first 0x80 pages (all of low memory)
    mov esi, 0x0
    mov edi, 0x0
    mov cx, 0x80
    mov ebx, eax
    mov edx, ebx
    call map_contiguous_pages
    mov dword [avail_mem_start], edx

    ; Map 0x100000 to 0xC000000 (2M for the kernel)
    mov esi, 0x100000
    mov edi, 0xC000000
    mov cx, 0x200
    mov ebx, dword [page_table]
    call map_contiguous_pages
    mov dword [avail_mem_start], edx

    mov eax, dword [avail_mem_start]
    mov dword [boot_info.avail_low_mem_start], eax
    mov eax, dword [avail_mem_end]
    mov dword [boot_info.avail_low_mem_end], eax

    ; Mask all IRQs
    mov al, 0xFF
    out 0xA1, al
    out 0x21, al
    nop
    nop

    lidt [idt]

    mov eax, CR4_PAE | CR4_PGE
    mov cr4, eax

    mov edx, dword [page_table]
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

    ; load_elf_binary(rsi: current_kernel_address)
    ; Virtual address to jump to will be returned in rax
    mov esi, [boot_info.kernel_physical_start]
    call load_elf_binary

    ; Set up kernel stack
    mov rsp, KERNEL_STACK

    ; Pass addr of boot_info to kmain (see kernel/main.h)
    mov rdi, boot_info
    jmp rax

    ; Infinite loop in case we ever return (shouldn't)
    cli
    hlt
    jmp $

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
.descriptor:
    .size    dw $ - gdt - 1 
    .address dd gdt

page_table: dd 0

boot_info:
    .num_memory_map_entries: dd 0
    .memory_map_base:        dd 0
    .avail_low_mem_start:    dd 0
    .avail_low_mem_end:      dd 0
    .kernel_physical_start:  dd 0
    .kernel_physical_end:    dd 0
    .kernel_boot_size:       dd 0
    .drive_number:           dd 0

CODE_SEG equ gdt.code - gdt
DATA_SEG equ gdt.data - gdt

LOW_MEM_END equ 0x80000
PAGE_SIZE equ 0x1000

KERNEL_BASE  equ 0xC000000
KERNEL_STACK equ KERNEL_BASE + 0x200000

PAGE_PRESENT equ (1 << 0)
PAGE_WRITE   equ (1 << 1)
PAGE_LARGE   equ (1 << 7)

CR0_PAGE equ (1 << 31)
CR0_PROT equ (1 << 0)
CR4_PAE equ (1 << 5) 
CR4_PGE equ (1 << 7)
LME equ 0x100
