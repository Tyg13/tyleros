[BITS 16]

TEXT_BUFFER_ADDR equ 0xB8000
TEXT_BUFFER_SEG equ TEXT_BUFFER_ADDR >> 4
NL equ 0xA

COLUMNS_PER_LINE equ 80

; bp <- String ptr (null-terminated)
global vga_print
vga_print:
    push bx
    push cx
    push dx
    push es

    mov bx, TEXT_BUFFER_SEG
    mov es, bx

    movzx bx, byte [ds:current_column]
    movzx cx, byte [ds:current_line]
.loop:
    ; al = c
    ; bp = str pointer
    ; bx = current_column
    ; cx = current_line

    ; load c from input string, exit if '\0'
    mov al, [ds:bp]
    and al, al
    jz .exit

    ; check for newline
    cmp ax, NL
    je .handle_nl

    ; Not a newline, write actual char to screen
    ; at position `80 * line + column`.
    ;
    ; Note that `x * 80 = 16x + 64x`
    mov di, cx ; line
    mov dx, cx ; line
    shl di, 4  ; line << 4 == 16 * line
    shl dx, 6  ; line << 6 == 64 * line
    add di, dx ; 16 * line + 64 * line (80 * line)
    add di, bx ; 80 * line + column
    shl di, 1  ; di = 2 * (80 * line + column)
    mov [es:di], al
    inc di
    mov al, 0x07
    mov [es:di], al

    ; current_column += 1
    inc bx

    ; current_line = current_column >= 0x80 : current_line + 1 : current_line;
    ; current_column = current_column >= 0x80 : 0 : current_column;
    mov dx, cx
    inc dx
    xor ax, ax
    cmp bx, COLUMNS_PER_LINE
    cmovge bx, ax
    cmovge cx, dx

.nl_handled:
    inc bp
    jmp .loop

.exit:
    mov [ds:current_column], bx
    mov [ds:current_line], cx
    pop es
    pop dx
    pop cx
    pop bx
    ret

.handle_nl:
    ; ++current_line, current_column = 0
    inc cx
    xor bx, bx
    jmp .nl_handled

current_line: db 0
current_column: db 0

; clobbers: cx, di, bp
global print_init
print_init:
    push es
    push ax

    ; set 80x60 mode
    mov ax, 0x001A
    int 0x10

    mov ax, TEXT_BUFFER_SEG
    mov es, ax

    xor di, di
    mov cx, 80 * 60 * 2
    xor ax, ax
    rep stosb

    pop ax
    pop es

    mov bp, .init_msg
    jmp vga_print

.init_msg: db "screen: initialized", NL, 0

global got_here_msg
got_here_msg: db "got here", NL, 0
