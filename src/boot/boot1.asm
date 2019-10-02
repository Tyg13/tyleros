[BITS 16]

boot:
    ; Copy this boot sector to 0x0500
    mov si, 0x7c00
    mov di, 0x0500
    mov cx, 0x100
    rep movsd

    mov bx, .relocated
    jmp bx
.relocated:
    call read_floppy_track

    ; Jump to stage 2
    mov bx, 0x7c00
    jmp bx

read_floppy_track:
    mov ah, 2
    mov al, 16
    xor ch, ch
    mov cl, 2
    xor dx, dx
    mov es, dx
    mov bx, 0x7c00
    int 0x13
    ret

times 510 - ($ - $$) db 0 ; Boot sector is 512 bytes
dw 0xAA55 ; Boot signature
