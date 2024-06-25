[BITS 16]

PAGE_SIZE equ 0x1000

%include "utils.asm"

global avail_mem_start
global avail_mem_end
global low_mem_size

avail_mem_start: dd 0
avail_mem_end:   dd 0
low_mem_size:    dd 0

; Allocate a `cx` zero'd pages from the available memory (set in stage1)
; return address in `eax`
; clobbers: cx, dx, di
global allocate_pages
allocate_pages:
    push ebx

    mov ebx, dword [avail_mem_start]
    mov edx, dword [avail_mem_end]
    cmp ebx, edx
    jge .fail ; TODO handle allocation failure

    ; dx = num pages
    mov dx, cx

    ; save allocated start for return
    push ebx
    ; save segment register
    push es

    ; Convert ebx -> es:di
    set_segment_and_base es, di, ebx

.next_page:
    ; Zero [es:di] -> [es:di + cx]
    xor ax, ax
    mov cx, PAGE_SIZE
    rep stosb

    ; Inc avail mem start by one page
    add dword [avail_mem_start], PAGE_SIZE

    dec dx
    jnz .next_page

    ; restore segment register
    pop es
    ; restore allocated start for return
    pop eax

    pop ebx
    ret
.fail:
    int 3
