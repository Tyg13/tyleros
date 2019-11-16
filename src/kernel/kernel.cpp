#include "kernel.h"

#include "cmos.h"
#include "dma.h"
#include "filesystem.h"
#include "floppy.h"
#include "gdt.h"
#include "idt.h"
#include "memory.h"
#include "scheduler.h"
#include "sse.h"
#include "timing.h"
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
   init_timer();
   init_floppy_driver();
   init_dma();
   init_filesystem();
   enable_task_switch();
   while(true) {   
      asm volatile ("pause");
   }
}
