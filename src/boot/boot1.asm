[BITS 16]

boot:
    ; dl contains the drive number, set by the BIOS
    mov [drive_number], dl

    ; Copy this boot sector to 0x0500
    mov si, 0x7c00
    mov di, 0x0500
    mov cx, 0x100
    rep movsd

    mov bx, .relocated
    jmp bx
.relocated:
    ; Read next boot sector to 0x7c00
    mov bx, 0x7c00
    mov cl, 2
    mov al, 1
    call read_floppy_disk

    ; Read 28KiB (0x7000 bytes) of kernel to 0x50000
    mov bx, 0x5000
    mov es, bx
    xor bx, bx
    mov cl, 3
    mov al, 56
    call read_floppy_disk

    mov bp, es
    shl ebp, 4
    add ebp, ebx

    ; Jump to stage 2
    mov bx, 0x7c00
    jmp bx

read_floppy_disk:
    mov ah, 2
    xor ch, ch
    xor dh, dh
    mov dl, [drive_number]
    int 0x13
    ret

drive_number: db 0

times 510 - ($ - $$) db 0 ; Boot sector is 512 bytes
dw 0xAA55 ; Boot signature
