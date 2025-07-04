; Stage 1 of the bootloader
; This stage relocates itself to elsewhere in low memory,
; reads the stage 2 bootloader from the floppy at 0x700, and
; jumps to it.
[BITS 16]

global _start
_start:

; Reserved area for the FAT, which gets filled during the build process.
FAT:
jmp short boot
BYTES_PER_SECTOR     equ FAT + 0x0B
SECTORS_PER_CLUSTER  equ FAT + 0x0D
NUM_RESERVED_SECTORS equ FAT + 0x0E
NUM_OF_FAT_TABLES    equ FAT + 0x10
ROOT_DIR_ENTRY_COUNT equ FAT + 0x11
SECTORS_PER_FAT      equ FAT + 0x16
SECTORS_PER_TRACK    equ FAT + 0x18
NUMBER_OF_HEADS      equ FAT + 0x1A
times 0x3E - ($$ - $) db 0

boot:
    ; Zero out segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Set up a stack 16-bit aligned to the end of low memory (0x7FFFF)
    mov ax, 0x7000
    mov ss, ax
    mov sp, 0xFFFE

    ; dl contains the drive number, set by the BIOS
    push dx

    ; Copy this boot sector to 0x0500
    mov si, 0x7c00
    mov di, 0x0500
    mov cx, 0x200
    rep movsb

    mov bx, .relocated
    jmp bx

.relocated:
    ; Read the stage2 bootloader from floppy to 0x700
    mov cx, 1
    mov bx, 0x700
    pop dx ; restore drive number
.read_sector:
    mov si, word [SECTORS_PER_TRACK]
    mov di, word [NUMBER_OF_HEADS]
    ; read_floppy_sector(sector: cx, base: bx, drive: dl, sectors_per_track: si, number_of_heads: di)
    call read_floppy_sector
    add bx, word [BYTES_PER_SECTOR]
    inc cx
    cmp cx, word [NUM_RESERVED_SECTORS]
    jl .read_sector

.sectors_all_read:
    ; Return end of stage2 in `eax`
    mov eax, ebx

    ; Jump to stage 2
    mov bx, 0x700
    jmp bx

%include "read_floppy.asm"

times 510 - ($ - $$) db 0 ; Boot sector is 512 bytes
dw 0xAA55 ; Boot signature
