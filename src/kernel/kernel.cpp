#include "kernel.h"

#include "idt.h"
#include "memory.h"

void kmain(void)
{
    load_idt();
    sort_memory_map();
loop:
    goto loop;
}
