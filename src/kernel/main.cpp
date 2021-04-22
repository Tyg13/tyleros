#include "main.h"

#include "cmos.h"
#include "dma.h"
#include "elf.h"
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
   idt::init();
   memory::init(boot->memory_map_base,
                boot->num_memory_map_entries,
                boot->avail_low_mem_start,
                boot->avail_low_mem_end);
   vga::init();
   scheduler::init();
   init_timer();
   init_floppy_driver();
   init_filesystem();
   scheduler::enable_task_switch();
   while(true) {   
      asm volatile ("pause");
   }
}
