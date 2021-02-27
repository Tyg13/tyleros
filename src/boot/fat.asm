[BITS 16]

PAGE_SIZE equ 0x1000 ; 1 << 12

BOOT_SECTOR equ 0x500

BYTES_PER_SECTOR     equ BOOT_SECTOR + 0x0B
SECTORS_PER_CLUSTER  equ BOOT_SECTOR + 0x0D
NUM_RESERVED_SECTORS equ BOOT_SECTOR + 0x0E
NUM_OF_FAT_TABLES    equ BOOT_SECTOR + 0x10
ROOT_DIR_ENTRY_COUNT equ BOOT_SECTOR + 0x11
SECTORS_PER_FAT      equ BOOT_SECTOR + 0x16
SECTORS_PER_TRACK    equ BOOT_SECTOR + 0x18
NUMBER_OF_HEADS      equ BOOT_SECTOR + 0x1A

SIZE_OF_DIRECTORY_ENTRY equ 0x20

extern read_floppy_sector

extern allocate_pages
extern avail_mem_start

; dl contains the drive number
global read_kernel_from_filesystem
read_kernel_from_filesystem:
    ; Store drive number
    push dx

    mov bp, sp

    ; Calculate root dir sectors
    ; root_dir_sectors = ((root_dir_entry_count * 32) + (bytes_per_sector - 1)) / bytes_per_sector)
    mov cx, word [ROOT_DIR_ENTRY_COUNT]
    shl cx, 5
    add cx, 0x200 - 1 ; 512 bytes per sector
    shr cx, 9         ; 512 = 2 ^ 9
    mov word [root_dir_sectors], cx

    ; Calculate the first data sector
    ; first_data_sector = (NUM_OF_FAT_TABLES * SECTORS_PER_FAT)
    ;                   + NUM_RESERVED_SECTORS + root_dir_sectors
    xor bx, bx
    mov bl, byte [NUM_OF_FAT_TABLES]
    mov ax, word [SECTORS_PER_FAT]
    mul bx
    add ax, word [root_dir_sectors]
    add ax, word [NUM_RESERVED_SECTORS]
    mov word [first_data_sector], ax

    ; first_root_dir_sector = (first_data_sector - root_dir_sectors)
    mov cx, ax
    sub cx, word [root_dir_sectors]
    mov word [first_root_dir_sector], cx

    ; Allocate root dir table
    ; pages = (root_dir_sectors * BYTES_PER_SECTOR) / PAGE_SIZE + 1
    mov cx, word [root_dir_sectors]
    mov ax, [BYTES_PER_SECTOR]
    mul cx
    shr ax, 12
    inc ax
    mov cx, ax
    call allocate_pages
    mov dword [root_dir_table], eax

    ; Allocate FAT table
    ; pages = (SECTORS_PER_FAT * BYTES_PER_SECTOR) / PAGE_SIZE + 1
    mov cx, word [SECTORS_PER_FAT]
    mov ax, word [BYTES_PER_SECTOR]
    mul cx
    shr ax, 12
    inc ax
    mov cx, ax
    call allocate_pages
    mov dword [fat_table], eax

    ; dl = drive number
    mov dx, word [bp]

    ; Read root directory table
    mov ebx, dword [root_dir_table]
    mov cx, word [first_root_dir_sector]
    mov ax, word [root_dir_sectors]
.read_root_dir_sector:
    mov si, [SECTORS_PER_TRACK]
    mov di, [NUMBER_OF_HEADS]
    call read_floppy_sector
    add ebx, [BYTES_PER_SECTOR]
    inc cx
    dec ax
    jnz .read_root_dir_sector

    ; Read fat table
    mov ebx, dword [fat_table]
    mov cx, [NUM_RESERVED_SECTORS]
    mov ax, [SECTORS_PER_FAT]
.read_fat_sector:
    mov si, [SECTORS_PER_TRACK]
    mov di, [NUMBER_OF_HEADS]
    call read_floppy_sector
    add ebx, [BYTES_PER_SECTOR]
    inc cx
    dec ax
    jnz .read_fat_sector

    ; Scan root directory table for our kernel image
    ; Start with first entry
    mov ebx, dword [root_dir_table]
    xor eax, eax
.check_entry_name:

    ; Check file name, is it our kernel's name?
    mov esi, kernel_name
    mov edi, ebx
    add edi, eax
    mov ecx, 8
    rep cmpsb

    ; If not, check the next entry
    je .found_kernel_entry
    add eax, SIZE_OF_DIRECTORY_ENTRY
    jmp .check_entry_name

.found_kernel_entry:
    ; Save kernel file entry address
    push bx

    ; Allocate pages for kernel image
    mov ax, [bx + ENTRY_SIZE_IN_BYTES]
    shr ax, 12
    inc ax
    mov cx, ax
    call allocate_pages
    mov dword [kernel_load_start], eax
    mov dword [kernel_load_end], eax

    ; Restore kernel file entry address
    pop bx

    ; Get cluster number of entry's first cluster
    lea bx, [bx + ENTRY_FIRST_CLUSTER]
    mov cx, word [bx]

.read_cluster:
    ; Store cluster number
    push cx

    xor ah, ah
    ; Sector will be ((cluster - 2) * sectors_per_cluster) + first_data_sector
    sub cx, 2
    mov al, byte [SECTORS_PER_CLUSTER]
    mul cx
    mov cx, ax
    add cx, word [first_data_sector]

    ; Read next sector
    mov ebx, dword [kernel_load_end]
    mov si, [SECTORS_PER_TRACK]
    mov di, [NUMBER_OF_HEADS]
    mov dx, word [bp]
    call read_floppy_sector
    add dword [kernel_load_end], 0x200

    ; Restore cluster number
    pop cx

    mov si, 1
    xor di, di
    ; Check if last cluster is odd
    test cx, 1
    cmovnz di, si

    ; fat_offset = cluster + (cluster / 2)
    mov ax, cx
    shr cx, 1
    add ax, cx

    ; Read next cluster number from the FAT
    ; Since this is FAT12, we read 16 bits and shift (or mask) to obtain the 12 bits we want.
    mov bx, word [fat_table]
    add bx, ax
    mov cx, word [bx]

    ; Check if cluster number was odd
    cmp di, 1
    jne .else
.if:
    ; If the cluster number was odd we want the top 12 bits.
    shr cx, 4
    jmp .done
.else:
    ; If it was even, we want the bottom 12 bits.
    and cx, 0x0FFF
.done:
    ; Check if there are more clusters (0xFF8 signals the end)
    cmp cx, 0xFF8
    jl .read_cluster

.no_more_clusters:
    ; Restore drive number
    pop dx
    mov eax, dword [kernel_load_start]
    mov edx, dword [kernel_load_end]
    ret


root_dir_sectors:      dw 0
first_data_sector:     dw 0
first_root_dir_sector: dw 0

fat_table:      dd 0
root_dir_table: dd 0

kernel_load_start: dd 0
kernel_load_end:   dd 0
kernel_name: db "KERNEL  "

ENTRY_FIRST_CLUSTER equ 0x1A
ENTRY_SIZE_IN_BYTES equ 0x1C
