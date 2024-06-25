[BITS 16]

%include "utils.asm"

PAGE_SIZE equ 0x1000
ENTRIES_PER_TABLE equ 512
SIZE_OF_ENTRY equ 0x8
LOG2_SIZE_OF_ENTRY equ 3

PREFIX_MASK equ ENTRIES_PER_TABLE - 1
PML1_MASK   equ PREFIX_MASK << 12
PML2_MASK   equ PREFIX_MASK << 21
PML3_MASK   equ ~0 << 30

PAGE_PRESENT equ (1 << 0)
PAGE_WRITE   equ (1 << 1)
PAGE_LARGE   equ (1 << 7)

extern allocate_page
extern vga_print

global page_tables_start
page_tables_start: dd 0
global page_tables_end
page_tables_end: dd 0

TRUE: dw 1

; Map `ecx` contiguous pages, starting at physical `esi`, to virtual `edi`
global map_contiguous_pages
map_contiguous_pages:
    xchg bx, bx
    cmp ecx, ENTRIES_PER_TABLE
    jg .fail

    mov bp, sp ; create stack frame
    sub sp, 6 * 4

    ; store initial offsets on the stack
    mov eax, edi
    and eax, PML1_MASK
    shr eax, 12
    shl eax, LOG2_SIZE_OF_ENTRY
    mov [bp - 1 * 4], eax ; <- pml1 offset (slot: 1)

    mov eax, edi
    and eax, PML2_MASK
    shr eax, 21
    shl eax, LOG2_SIZE_OF_ENTRY
    mov [bp - 2 * 4], eax ; <- pml2 offset (slot: 2)

    mov eax, edi
    and eax, PML3_MASK
    shr eax, 30
    shl eax, LOG2_SIZE_OF_ENTRY
    mov [bp - 3 * 4], eax ; <- pml3 offset (slot: 3)

    ; Check if PML4 table hasn't been allocated yet, if so allocate it
    xor ax, ax
    mov ebx, dword [page_tables_start]
    cmp ebx, dword [page_tables_end]
    cmove ax, [TRUE]
    call allocate_page_if_needed
    ; ebx holds PML4 base

    ; Get PML4 entry (PML3 pointer)
    ; assume PML4 offset is zero
    call get_or_allocate_level ; ebx <- PML3 pointer

    ; Get PML3 entry (PML2 pointer)
    add ebx, [bp - 3 * 4] ; PML3 offset
    call get_or_allocate_level ; ebx <- PML2 pointer
    mov [bp - 4 * 4], ebx ; (slot: 4) <- PML2 pointer

    ; Get PML2 entry (PML1 pointer)
    add ebx, [bp - 2 * 4] ; PML2 offset
    call get_or_allocate_level ; ebx <- PML1 pointer
    mov [bp - 5 * 4], ebx ; (slot: 5) <- PML1 pointer

    ; Get offset of first page to write
    add ebx, [bp - 1 * 4] ; PML1 offset

    ; Add attributes to the virtual address
    or esi, PAGE_PRESENT | PAGE_WRITE

.write_table:
    ; keep track of how many entries in we are to the PML1 table
    mov edx, [bp - 1 * 4] ; PML1 offset
    shr edx, LOG2_SIZE_OF_ENTRY

    set_segment_and_base es, di, ebx
.write_page_table_entry:
    mov dword [es:di], esi
    add esi, PAGE_SIZE
    add di, SIZE_OF_ENTRY
    inc edx
    cmp edx, ecx
    je .done ; wrote all the page entries we wanted to

    cmp edx, ENTRIES_PER_TABLE
    jge .table_full ; hit this table's max capacity

    jmp .write_page_table_entry ; write the next entry

.done:
    mov sp, bp ; cleanup stack frame
    ret

.table_full:
    ; allocate the next table in PML2

    ; advance PML2 by one entry
    mov ebx, [bp - 2 * 4] ; PML2 offset
    add ebx, SIZE_OF_ENTRY
    mov [bp - 2 * 4], ebx

    ; Allocate next table
    call allocate_page

    ; Write the table into PML2
    mov eax, [bp - 2 * 4] ; PML2 offset
    mov edx, [bp - 4 * 4] ; PML2 pointer
    add edx, eax
    set_segment_and_base es, di, edx
    or ebx, PAGE_PRESENT | PAGE_WRITE
    mov dword [es:di], ebx

    sub ecx, ENTRIES_PER_TABLE

    jmp .write_table

.fail:
    int3

; params: ebx <- pointer to entry in level N table
; returns: ebx <- valid address to a level N-1 table
; clobbers: eax
get_or_allocate_level:
    push ebp
; get the entry, and check if it's marked present already
    set_segment_and_base es, bp, ebx
    mov eax, dword [es:bp]
    test eax, PAGE_PRESENT
    jnz .present

; entry address not present: allocate a page for it, store it in the level N
; table, and return that address
    push es
    push bp
    call allocate_page ; addr returned in ebx
    mov eax, ebx
    or eax, PAGE_PRESENT | PAGE_WRITE
    pop bp
    pop es
    mov dword [es:bp], eax
    pop ebp
    ret

.present:
    and eax, ~PREFIX_MASK
    mov ebx, eax
    pop ebp
    ret

; params: ax -> whether or not to allocate
; returns: ebx <- (if ax == TRUE) valid address for one page
; clobbers: ax (if ax == TRUE)
allocate_page_if_needed:
    test ax, ax
    jz .done

    call allocate_page

.done:
    ret

; returns: ebx <- valid address for one page
; clobbers: ax
allocate_page:
    push es
    push di
    push cx

    mov ebx, dword [page_tables_end]
    set_segment_and_base es, di, ebx
    xor ax, ax
    mov cx, PAGE_SIZE
    rep stosb
    mov edx, ebx
    add edx, PAGE_SIZE
    mov dword [page_tables_end], edx

    pop cx
    pop di
    pop es

    ret
