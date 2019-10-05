#include "kernel.h"

#include <stdint.h>
#include <stddef.h>

#include "idt.h"

void kmain(void)
{
    load_idt();
loop:
    goto loop;
}
