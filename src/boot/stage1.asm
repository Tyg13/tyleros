; Stage 1 of the bootloader
; This stage relocates itself to elsewhere in low memory,
; reads the stage 2 bootloader from the floppy at 0x700, and
; jumps to it.
[BITS 16]

; Reserved area for the FAT, which gets filled during the build process.
global _start
_start:
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

drive_number: db 0
boot:
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
    pop dx
.read_sector:
    mov si, [SECTORS_PER_TRACK]
    mov di, [NUMBER_OF_HEADS]
    ; read_floppy_sector(sector: cx, base: bx, drive: dl, sectors_per_track: si, number_of_heads: di)
    call read_floppy_sector
    add bx, [BYTES_PER_SECTOR]
    cmp cx, NUM_STAGE2_SECTORS
    inc cx
    jb .read_sector

    ; Return end of stage2 in `ax`
    mov ax, bx

    ; Jump to stage 2
    mov bx, 0x700
    jmp bx

%include "read_floppy.asm"

times 510 - ($ - $$) db 0 ; Boot sector is 512 bytes
dw 0xAA55 ; Boot signature

NUM_STAGE2_SECTORS equ 10
