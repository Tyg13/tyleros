[BITS 16]

FAT:
jmp short boot
times 0x60 - ($$ - $) db 0
boot:
    ; dl contains the drive number, set by the BIOS
    mov [drive_number], dl

    ; Copy this boot sector to 0x0500
    mov si, 0x7c00
    mov di, 0x0500
    mov cx, 0x100
    rep movsw

    mov bx, .relocated
    jmp bx
.relocated:

    ; Calculate the first data sector
    ; root_dir_sectors  = ((root_dir_entry_count * 32) + (bytes_per_sector - 1)) / bytes_per_sector)
    mov cx, word [ROOT_DIR_ENTRY_COUNT]
    shl cx, 5
    add cx, 0x200 - 1 ; 512 bytes per sector
    shr cx, 9         ; same as dividing by 512

    ; Save root_dir_sectors in si
    mov si, cx

    ; first_data_sector = (num_of_fat_tables * sectors_per_table) + root_dir_sectors + reserved_sectors
    mov bl, byte [NUM_OF_FAT_TABLES]
    mov ax, word [SECTORS_PER_FAT]
    mul bl
    add ax, si
    add ax, word [NUM_RESERVED_SECTORS]

    ; Save first data sector
    mov byte [first_data_sector], al

    mov cx, ax
    sub cx, si
    mov byte [first_root_dir_sector], cl

    ; Start with first entry
    xor si, si

.read_fat_entry:
    mov cx, word [first_root_dir_sector]

    ; Read root directory table to 0x700
    mov ebx, 0x700
    call read_floppy_sector

    ; Get first byte of entry
    add bx, si
    mov ax, word [bx]
    test al, al
    jz .no_more_entries

    ; Unused entry
    cmp ax, 0xE5
    je .next_entry

    ; Save entry offset
    push si

    ; Get cluster number of entry's first cluster
    lea bx, [bx + CLUSTER_OFFSET]
    mov cx, word [bx]

.next_cluster:
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
    mov ebx, dword [where_to_load_file]
    call read_floppy_sector
    add dword [where_to_load_file], 0x200

    ; Restore cluster number
    pop cx

.unused_entry:
    mov si, 1
    xor di, di
    ; Check if last cluster is odd
    test cx, 1
    cmovnz di, si

    ; fat_offset = cluster + (cluster / 2)
    mov ax, cx
    shr cx, 1
    add ax, cx

    ; fat_sector = first_fat_sector + (fat_offset / sector_size)
    ; ent_offset = fat_offset % sector_size
    xor dx, dx
    mov cx, word [NUM_RESERVED_SECTORS]
    mov bx, word [BYTES_PER_SECTOR]
    div bx
    add cx, ax

    ; Save next cluster entry offset
    push dx

    ; Read FAT table
    mov ebx, 0x700
    call read_floppy_sector

    ; Read next cluster number
    pop dx
    add bx, dx
    mov cx, word [bx]

    cmp di, 1
    jne .else
.if:
    shr cx, 4
    jmp .done
.else:
    and cx, 0x0FFF
.done:
    ; Check if there are more clusters
    cmp cx, 0xFF8
    jl .next_cluster

    ; Restore entry offset
    pop si
.next_entry:
    mov bx, 0x700
    ; Entries are 32 bytes wide
    add si, 32

    mov dword [where_to_load_file], 0x10000
    jmp .read_fat_entry

.no_more_entries:
    mov ebp, 0x10000

    ; Jump to stage 2
    mov bx, 0x7c00
    jmp bx

read_floppy_sector:
    push ebx
    ; ebx contains transfer base, to be converted to [es:bx]
    mov eax, ebx
    shr eax, 4
    mov es, ax
    and ebx, 0xF
    push bx

    ; cl contains desired linear sector (LBA)
    xor ch, ch

    ; Translate to CHS
    ; track  = (sectors / sectors_per_track)
    ; sector = (sectors % sectors_per_track) + 1
    ; (since sectors are numbered from 1)
    mov ax, cx
    xor dx, dx
    mov bx, [SECTORS_PER_TRACK]
    div bx
    inc dx
    ; DX contains the sector within the track we're in
    ; AX contains which track we're on
    mov cl, dl

    ; cylinder = (track / number_of_heads)
    ; head     = (track % number_of_heads)
    xor dx, dx
    mov bp, [NUMBER_OF_HEADS]
    div bp
    ; DX contains which head we're on
    ; AX contains which cylinder we're on

    mov ch, al ; ch = cylinder 0xff

    shr ax, 2
    and al, 0xC0
    or cl, al  ; cl = sector | ((cylinder >> 2) & 0xC0)

    mov dh, dl             ; dh = head
    mov dl, [drive_number] ; dl = drive_number

    pop bx    ; es:bx = transfer base
.retry:
    mov al, 1 ; al = Total sector count
    mov ah, 2 ; al = Read
    int 0x13
    jc .retry
    test ah, ah
    jnz .retry
    pop ebx
    ret

drive_number: db 0

first_data_sector:     db 0
first_root_dir_sector: db 0

current_table_value: dw 0x0

where_to_load_file: dd 0x7c00

times 510 - ($ - $$) db 0 ; Boot sector is 512 bytes
dw 0xAA55 ; Boot signature
BYTES_PER_SECTOR     equ FAT + 0x0B
SECTORS_PER_CLUSTER  equ FAT + 0x0D
NUM_RESERVED_SECTORS equ FAT + 0x0E
NUM_OF_FAT_TABLES    equ FAT + 0x10
ROOT_DIR_ENTRY_COUNT equ FAT + 0x11
SECTORS_PER_FAT      equ FAT + 0x16
SECTORS_PER_TRACK    equ FAT + 0x18
NUMBER_OF_HEADS      equ FAT + 0x1A

CLUSTER_OFFSET       equ 0x1A
