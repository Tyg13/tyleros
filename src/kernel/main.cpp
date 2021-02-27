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

void kmain(boot_info* boot)
{
   enable_sse();
   serial::init();
   gdt::init();
   IDT::init();
   memory::init(boot->memory_map_base,
                boot->num_memory_map_entries,
                boot->avail_low_mem_start,
                boot->avail_low_mem_end);
   vga::clear_screen();
   init_scheduler();
   init_timer();
   init_floppy_driver();
   init_filesystem();
   enable_task_switch();
   while(true) {   
      asm volatile ("pause");
   }
}
