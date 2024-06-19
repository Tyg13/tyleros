[BITS 16]

TEXT_BUFFER_ADDR equ 0xB8000
TEXT_BUFFER_SEG equ TEXT_BUFFER_ADDR >> 4

; bp <- String ptr (null-terminated)
; clobbers: bx, cx
global vga_print
vga_print:
    push bx
    push cx
    push es

    push TEXT_BUFFER_SEG
    pop es

    movzx cx, [ds:current_column]
    mov bx, cx
    shl bl, 1

.loop:
    mov al, [ds:bp]
    and al, al
    jz .exit
    mov [es:bx], al
    inc bl
    mov al, 0x07
    mov [es:bx], al
    inc bl
    inc cl
    inc bp
    jmp .loop

.exit:
    mov bl, [current_column]
    pop es
    pop cx
    pop bx
    ret

current_line: db 0
current_column: db 0

global print_init
print_init:
    mov ax, 0x001A
    int 0x10

    push es
    push TEXT_BUFFER_SEG
    pop es

    xor di, di
    mov cx, 0x1000
    xor ax, ax
    rep stosb

    pop es

    mov bp, .init_msg
    jmp vga_print

.init_msg: db "screen: initialized", 0
