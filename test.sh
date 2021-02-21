#!/bin/bash

GDB_COMMANDS=(
    "file bin/kernel.debug.elf"
    "target remote :1234"
)

GDB_ARGS=()
for c in "${GDB_COMMANDS[@]}"; do
    GDB_ARGS+=("-iex" "$c")
done

#bochs -q &>/dev/null &
qemu-system-x86_64 -s -S -drive if=floppy,format=raw,readonly=on,file=bin/boot.img &
cgdb -d gdb "${GDB_ARGS[@]}"
