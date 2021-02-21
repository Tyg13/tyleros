#include "main.h"

#include "cmos.h"
#include "dma.h"
#include "filesystem.h"
#include "floppy.h"
#include "gdt.h"
#include "idt.h"
#include "memory.h"
#include "scheduler.h"
#include "serial.h"
#include "sse.h"
#include "timing.h"
#include "util.h"
#include "vga.h"

void kmain()
{
   vga::clear_screen();
   enable_sse();
   IDT::init();
   serial::init();
   init_gdt();
   init_memory();
   init_scheduler();
   init_timer();
   init_floppy_driver();
   init_filesystem();
   enable_task_switch();
   while(true) {   
      asm volatile ("pause");
   }
}
