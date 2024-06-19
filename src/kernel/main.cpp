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
#include "timing.h"
#include "util.h"
#include "vga.h"

void kmain(boot_info *boot) {
  serial::init();
  gdt::init();
  idt::init();
  memory::init(boot->memory_map_base, boot->num_memory_map_entries,
               boot->avail_low_mem_start, boot->avail_low_mem_end);
  debug::printf("mem map: initialized\n");
  vga::init();
  kprintf("vga:       initialized\n");
  scheduler::init();
  kprintf("scheduler: initialized\n");
  init_timer();
  kprintf("timer:     initialized\n");
  init_floppy_driver();
  kprintf("floppy:    initialized\n");
  init_filesystem();
  kprintf("fs:        initialized\n");
  init_filesystem();
  scheduler::enable_task_switch();
  while (true) {
    asm volatile("pause");
  }
}
