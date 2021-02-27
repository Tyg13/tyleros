[BITS 16]

MEMORY_MAP_ENTRY_BASE_LO   equ 0x0  ; dd
MEMORY_MAP_ENTRY_BASE_HI   equ 0x4  ; dd
MEMORY_MAP_ENTRY_LENGTH_LO equ 0x8  ; dd
MEMORY_MAP_ENTRY_LENGTH_HI equ 0xC  ; dd
MEMORY_MAP_ENTRY_TYPE   equ 0x10 ; dd

SIZE_OF_MEMORY_MAP_ENTRY equ 24

MEMORY_MAP_ENTRY_USABLE equ 0x1

MEMORY_MAP_MAGIC equ 'PAMS' ; SMAP (system map) in LE order

TWO_MEGS equ 0x200000

global build_memory_map
build_memory_map:
    ; ax = MEMORY_MAP_BASE

    ; ebp = num memory map entries
    xor ebp, ebp

    ; di = MEMORY_MAP_BASE
    mov di, ax

    xor ax, ax
    mov es, ax

    xor ebx, ebx
    mov edx, MEMORY_MAP_MAGIC
.entry:
    mov eax, 0xE820
    mov ecx, 24
    int 0x15
    jc .done

    mov edx, MEMORY_MAP_MAGIC
    cmp eax, edx
    jne .done

    inc ebp

    test ebx, ebx
    je .done

    add di, 24

    jmp .entry

.done:
    mov eax, ebp
    ret

; Not needed right now, I think, but still useful
; Also needs adjustment due to the removal of some module data
;global find_first_usable_region_at_least_2mb
;find_first_usable_region_at_least_2mb:
;    mov si, [memory_map_base]
;    mov cx, [si + MEMORY_MAP_NUM_ENTRIES]
;    add si, MEMORY_MAP_ENTRIES
;
;    ; si = memory_map_entry*
;    ; cx = num_memory_map_entries
;    ; eax = returned address base
;    xor eax, eax
;.check_entry:
;
;    ; Check if we're out of entries
;    cmp cx, 0
;    je .done
;
;    ; Skip if this isn't a usable entry
;    mov ebx, dword [si + MEMORY_MAP_ENTRY_TYPE]
;    cmp ebx, MEMORY_MAP_ENTRY_USABLE
;    jne .next_entry
;
;    ; Skip if this is based at 0x0
;    mov ebx, dword [si + MEMORY_MAP_ENTRY_BASE_LO]
;    cmp ebx, 0
;    je .next_entry
;
;    ; Skip if this is based at 4G or higher
;    mov ebx, dword [si + MEMORY_MAP_ENTRY_BASE_HI]
;    cmp ebx, 0
;    jg .next_entry
;
;    ; Found a good entry if memory_len.lo >= 2M
;    mov ebx, dword [si + MEMORY_MAP_ENTRY_LENGTH_LO]
;    cmp ebx, TWO_MEGS
;    jge .good_entry
;
;    ; Found a good entry if memory_len.hi > 0
;    ; (since 4G >= 2M)
;    mov ebx, dword [si + MEMORY_MAP_ENTRY_LENGTH_HI]
;    cmp ebx, 0
;    jg .good_entry
;
;    ; Otherwise, go to the next
;    jmp .next_entry
;
;.good_entry:
;    mov eax, dword [si + MEMORY_MAP_ENTRY_BASE_LO]
;    jmp .done
;
;.next_entry:
;    dec cx
;    add si, SIZE_OF_MEMORY_MAP_ENTRY
;    jmp .check_entry
;
;.done:
;    ret
