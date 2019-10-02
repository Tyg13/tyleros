#include "kernel.h"

int global = 1;

void kmain(void)
{
loop:
    goto loop;
}
