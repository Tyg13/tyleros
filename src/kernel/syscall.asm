[BITS 64]

extern syscall_handler

; Syscalls use System V ABI
global syscall_interrupt
syscall_interrupt:
	mov r8, rsp
	call syscall_handler
	iretq
