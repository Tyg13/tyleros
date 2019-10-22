#include "kernel.h"

#include "memory.h"
#include "idt.h"
#include "sse.h"
#include "vga.h"

void kmain(void)
{
   vga::clear_screen();
   enable_sse();
   init_interrupts();
   init_memory();
loop:
   goto loop;
}
