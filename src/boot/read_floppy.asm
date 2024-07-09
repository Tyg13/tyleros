[BITS 16]

%include "utils.asm"

; Read the `cx`-th floppy sector to `ebx` from drive `dl`
; with `si` sectors per track and `di` heads
; clobbers: esi, edi
global read_floppy_sector
read_floppy_sector:
    push es
    push eax
    push ebx
    push ecx
    push edx

    mov byte [.drive], dl
    mov dword [.buffer_address], ebx

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
    mov byte [.sector], dl

    ; cylinder = (track / number_of_heads)
    ; head     = (track % number_of_heads)
    xor dx, dx
    div di
    ; DX contains which head we're on
    ; AX contains which cylinder we're on
    mov byte [.head], dl
    mov byte [.cylinder], al

    shr ax, 2
    and al, 0xC0

    or al, byte [.sector] ; sector |= ((cylinder >> 2) & 0xC0)
    mov byte [.sector], al

.retry:
    mov al, 1 ; al => Total sector count
    mov ah, 2 ; ah => Read (2)
    mov ch, byte [.cylinder]
    mov cl, byte [.sector]
    mov dh, byte [.head]
    mov dl, byte [.drive]
    mov ebx, dword [.buffer_address] 
    set_segment_and_base es, bx, ebx ; Set [es:bx] from ebx
    int 0x13 ; CF = error, AH = return code, AL = actual sectors read
    jc .retry
    test ah, ah
    jnz .retry
    cmp al, 1
    jne .retry

    pop edx
    pop ecx
    pop ebx
    pop eax
    pop es
    ret

.cylinder: db 0
.sector:   db 0
.head:     db 0
.drive:    db 0
.buffer_address: dd 0
