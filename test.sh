#!/bin/bash

GDB_COMMANDS=(
    "file bin/kernel.elf"
    "target remote :6001"
)

GDB_ARGS=()
for c in "${GDB_COMMANDS[@]}"; do
    GDB_ARGS+=("-iex" "$c")
done

#bochs -q &>/dev/null &
qemu-system-x86_64 -gdb tcp::6001 -S \
    -drive if=floppy,format=raw,readonly=on,file=bin/boot.img \
    -no-reboot -no-shutdown \
    -device isa-debug-exit \
    -serial file:debug.log &
gdb "${GDB_ARGS[@]}" "$@"
