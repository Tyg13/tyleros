[BITS 16]

%include "utils.asm"

; Read the `cx`-th floppy sector to `ebx` from drive `dl`
; with `si` sectors per track and `di` heads
global read_floppy_sector
read_floppy_sector:
    push es
    push ax
    push cx
    push ebx
    push dx

    mov ebp, esp
    push ebp

    ; cl contains desired linear sector (LBA) - 1
    xor ch, ch

    ; Translate to CHS
    ; track  = (LBA / sectors_per_track)
    ; sector = (LBA % sectors_per_track) + 1
    ; (since sectors are numbered from 1)
    mov ax, cx ; LBA
    xor dx, dx
    div si     ; sectors per track
    inc dx
    ; DX contains the sector within the track we're in
    ; AX contains which track we're on

    mov cl, dl

    ; cylinder = (track / number_of_heads)
    ; head     = (track % number_of_heads)
    xor dx, dx
    div di

    ; DX contains which head we're on
    ; AX contains which cylinder we're on

    mov ch, al ; ch = cylinder 0xff

    shr ax, 2
    and al, 0xC0
    or cl, al  ; cl = sector | ((cylinder >> 2) & 0xC0)

    mov dh, dl ; dh = head

    mov ax, word [ebp]
    mov dl, al ; dl = drive_number

    mov ebx, dword [ebp + 2] ; ebx = destination base

    ; Set [es:bx] from ebx
    set_segment_and_base es, bx, ebx

.retry:
    mov al, 1 ; al => Total sector count
    mov ah, 2 ; ah => Read (2)
    int 0x13
    jc .retry
    test ah, ah
    jnz .retry

    pop ebp
    pop dx
    pop ebx
    pop cx
    pop ax
    pop es
    ret
