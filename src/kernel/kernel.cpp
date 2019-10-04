#include "kernel.h"

#include <stdint.h>
#include <stddef.h>

#include "idt.h"

void kmain(void)
{
loop:
    load_idt();
    int j = 1;
    int i = 0;
    int k = j / i;
    (void) k;
    goto loop;
}
