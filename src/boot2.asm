[ORG 0x7c00]
[BITS 16]

boot:
    jmp $

times 510 - ($ - $$) db 0 ; Boot sector is 512 bytes
dw 0xAA55 ; Boot signature
