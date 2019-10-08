#include "kernel.h"

#include "idt.h"
#include "memory.h"
#include "sse.h"

void kmain(void)
{
    load_idt();
    sort_memory_map();
    enable_sse();
loop:
    goto loop;
}
