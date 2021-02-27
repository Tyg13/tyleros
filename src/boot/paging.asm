[BITS 16]

%include "utils.asm"

PAGE_SIZE equ 0x1000

PREFIX_MASK                 equ (1 << 9) - 1

PAGE_PRESENT equ (1 << 0)
PAGE_WRITE   equ (1 << 1)
PAGE_LARGE   equ (1 << 7)

extern allocate_page

; Map `cx` contiguous pages, starting at physical `esi`, to virtual `edi`
; with page tables located from `ebx` to `edx`
; WARNING: will overrun if `cx` overflows the page table
global map_contiguous_pages
map_contiguous_pages:
    ; [bp + 4 * 3] = page_table_offset
    shr edi, 12
    mov eax, edi
    and eax, PREFIX_MASK
    push eax

    ; [bp + 4 * 2] = page_directory_offset
    shr edi, 9
    mov eax, edi
    and eax, PREFIX_MASK
    push eax

    ; [bp + 4 * 1] = page_directory_pointer_offset
    shr edi, 9
    mov eax, edi
    and eax, PREFIX_MASK
    push eax

    ; [bp + 4 * 0] = page_map_level_4_offset
    shr edi, 9
    mov eax, edi
    and eax, PREFIX_MASK
    push eax

    mov bp, sp

    ; save num pages
    push cx

    ; Check if PML4 table hasn't been allocated yet, if so allocate it
    mov edi, ebx
    call allocate_table_if_needed

    ; Get PML4 entry
    mov ebx, dword [bp + 4 * 0]
    shl ebx, 3
    add edi, ebx
    push edi
    set_segment_and_base es, di, edi
    mov eax, dword [es:di]
    pop edi
    call allocate_level_if_needed

    ; Get PDPT entry
    mov ebx, dword [bp + 4 * 1]
    shl ebx, 3
    add edi, ebx
    push edi
    set_segment_and_base es, di, edi
    mov eax, dword [es:di]
    pop edi
    call allocate_level_if_needed

    ; Get PDT entry
    mov ebx, dword [bp + 4 * 2]
    shl ebx, 3
    add edi, ebx
    push edi
    set_segment_and_base es, di, edi
    mov eax, dword [es:di]
    pop edi
    call allocate_level_if_needed

    ; Get PT offset
    mov ebx, dword [bp + 4 * 3]
    shl ebx, 3
    add edi, ebx
    ; edi = page table base
    ; esi = physical base
    ; cx = num pages (entries)
    pop cx
    mov eax, esi
    or ax, PAGE_PRESENT | PAGE_WRITE
    set_segment_and_base es, di, edi
.entry:
    mov dword [es:di], eax
    add di, 8
    add eax, 0x1000
    dec cx
    jnz .entry

    add sp, 4 * 4
    ret

allocate_table_if_needed:
    cmp edi, edx
    jl .done

    ; store table base
    push edi

    ; Zero out table
    set_segment_and_base es, di, edi
    xor ax, ax
    mov cx, PAGE_SIZE >> 1
    rep stosw

    ; store new page tables end
    add edx, PAGE_SIZE

    ; restore table base
    pop edi
.done:
    ret

; Check if `eax` points to an empty entry, and if so,
; allocates a new page level and points it to that.
; Finally, it stores the valid entry in `edi`
allocate_level_if_needed:
    cmp eax, 0
    jne .done

    ; Allocate a new page table, and store it as this
    ; entry's address
    set_segment_and_base es, di, edi
    mov eax, edx
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov dword [es:di], eax

    ; Also store the base address so we can return it
    push edx

    ; Zero out table
    set_segment_and_base es, di, edx
    xor ax, ax
    mov cx, PAGE_SIZE >> 1,
    rep stosw

    ; Adjust page tables end
    add edx, PAGE_SIZE

    ; Restore new page table base
    pop eax
.done:
    mov edi, eax
    and di, 0xF000
    ret
