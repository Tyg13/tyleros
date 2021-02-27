; Convert %3 to [%1:%2]
%macro set_segment_and_base 3 ; sreg, wreg, dreg
    push eax
    push ecx
    push edx

    mov eax, %3
    shr eax, 4
    mov %1, ax
    mov eax, %3
    and eax, 0xF
    mov %2, ax

    pop edx
    pop ecx
    pop eax

%endmacro
