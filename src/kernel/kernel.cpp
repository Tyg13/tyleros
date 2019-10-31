#include "kernel.h"

#include "gdt.h"
#include "idt.h"
#include "memory.h"
#include "scheduler.h"
#include "sse.h"
#include "util.h"
#include "vga.h"

#include <stdio.h>



void kmain(void)
{
   vga::clear_screen();
   enable_sse();
   init_gdt();
   init_memory();
   init_scheduler();
   init_interrupts();
loop:
   goto loop;
}
