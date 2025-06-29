#include "main.h"

#include "acpi.h"
#include "alloc.h"
#include "cmos.h"
#include "elf.h"
#include "filesystem.h"
#include "floppy.h"
#include "gdt.h"
#include "idt.h"
#include "input.h"
#include "low_memory_allocator.h"
#include "memory.h"
#include "minishell.h"
#include "paging.h"
#include "scheduler.h"
#include "serial.h"
#include "thunk.h"
#include "timing.h"
#include "vga.h"

#include "libadt/hash_map.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

extern "C" uint32_t crc32(void *data, uint32_t len);

__attribute__((section(".canary_begin"))) const char CANARY_BEGIN[] =
    "KERNEL START";
__attribute__((section(".canary_end"))) const char CANARY_END[] = "KERNEL END";

void kmain(const boot_info &b) {
  boot_info boot = b;
  serial::init();
  gdt::init();
  idt::init();

  memory::early_init(boot.memory_map_base, boot.num_memory_map_entries);
  paging::early_init();
  low_memory::init(boot.avail_low_mem_start, boot.avail_low_mem_end);

  // Need to put this here so it doesn't get corrupted
  // TODO: why does finishing paging init clobber the kernel image?
  const uint32_t computed_crc32 = crc32(
      (void *)(uintptr_t)boot.kernel_image_start, boot.kernel_image_size);

  paging::enable_kernel_page_protection(boot.kernel_stack_base);
  memory::finish_init();
  paging::finish_init();

  fprintf(stderr, "kernel crc=0x%x, size=%x\n", computed_crc32,
          boot.kernel_image_size);
  assert(boot.kernel_expected_crc32 == computed_crc32 &&
         "kernel integrity check failed!");

  vga::init();
  acpi::init();

  assert(memcmp(CANARY_BEGIN, "KERNEL START", sizeof(CANARY_BEGIN)) == 0);
  assert(memcmp(CANARY_END, "KERNEL END", sizeof(CANARY_END)) == 0);

  cmos::init();
  scheduler::init();
  init_timer();
  puts("timer:     initialized");
  init_floppy_driver(boot.drive_number);
  puts("floppy:    initialized");
  fs::init();
  elf::init();
  keyboard::init();
  scheduler::enable_task_switch();

  minishell::loop();
}
