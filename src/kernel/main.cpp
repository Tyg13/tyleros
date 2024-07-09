#include "main.h"

#include "assert.h"
#include "debug.h"
#include "filesystem.h"
#include "floppy.h"
#include "gdt.h"
#include "idt.h"
#include "input.h"
#include "io.h"
#include "low_memory_allocator.h"
#include "memory.h"
#include "paging.h"
#include "scheduler.h"
#include "serial.h"
#include "timing.h"
#include "util.h"
#include "vga.h"

#include <string.h>

extern "C" uint32_t crc32(void *data, uint32_t len);

static void loop();

void kmain(const boot_info &b) {
  boot_info boot = b;
  serial::init();
  gdt::init();
  idt::init();
  low_memory::init(boot.avail_low_mem_start, boot.avail_low_mem_end);

  paging::early_init();
  vga::init();

  // Need to put this here so it doesn't get corrupted
  // TODO: why does finishing paging init clobber the kernel image?
  const uint32_t computed_crc32 = crc32(
      (void *)(uintptr_t)boot.kernel_physical_start, boot.kernel_boot_size);

  paging::finish_init();

  debug::printf("kernel crc=0x%x, size=%x\n", computed_crc32,
                boot.kernel_boot_size);
  assert(boot.kernel_expected_crc32 == computed_crc32 &&
         "kernel integrity check failed!");

  memory::init(boot.memory_map_base, boot.num_memory_map_entries);
  // elf::init(boot.kernel_physical_start, boot.kernel_physical_end);
  scheduler::init();
  init_timer();
  debug::puts("timer:     initialized");
  init_floppy_driver(boot.drive_number);
  debug::puts("floppy:    initialized");
  fs::init();
  keyboard::init();
  scheduler::enable_task_switch();
  loop();
}

void loop() {
  const cpuid_info ret = cpuid(0);
  char processor_name[13];
  *(uint32_t *)(&processor_name[0]) = ret.ebx;
  *(uint32_t *)(&processor_name[4]) = ret.edx;
  *(uint32_t *)(&processor_name[8]) = ret.ecx;
  processor_name[12] = '\0';
  debug::printf("Running on %s\n", processor_name);

  vga::string::print("> ");

  char command_buffer[keyboard::BUFFER_CAPACITY] = {0};
  unsigned buffer_count = 0;

  using lock_type = decltype(keyboard::buffer.lock());
  const auto with_locked_buffer = [&](lock_type &buffer) {
    bool enter_seen = false;
    for (const char c : *buffer) {
      if (c == '\n') {
        enter_seen = true;
        break;
      }
      if (c == '\b' && buffer_count == 0)
        continue;

      if (c == '\b') {
        buffer_count -= 1;
      } else {
        command_buffer[buffer_count++] = c;
      }
      vga::string::print_char(c);
    }
    buffer->clear();

    if (!enter_seen)
      return;

    vga::current_cursor.lock()->newline();
    command_buffer[buffer_count] = '\0';

    if (strcmp(command_buffer, "help") == 0) {
      vga::string::puts("commands: help, clear, pages, ls, shutdown");
    } else if (strcmp(command_buffer, "clear") == 0) {
      vga::clear_screen();
      vga::current_cursor.lock()->reset();
    } else if (strcmp(command_buffer, "pages") == 0) {
      scheduler::schedule_task(
          +[](void *) { paging::kernel_page_tables.dump(); });
    } else if (strcmp(command_buffer, "ls") == 0) {
      fs::dump_dir("/");
    } else if (strcmp(command_buffer, "shutdown") == 0 ||
               strcmp(command_buffer, "s") == 0) {
      // QEMU magic shutdown
      io::outb(0x501, 0x42);
    } else {
      vga::string::print("error: `");
      vga::string::print(command_buffer);
      vga::string::puts("`: command not recognized");
    }

    // clear command buffer
    memset(command_buffer, 0, sizeof(command_buffer));
    buffer_count = 0;
    vga::string::print("> ");
  };

  while (true) {
    if (auto buffer = keyboard::buffer.try_lock()) {
      interrupts::disable();
      if (!(*buffer)->empty())
        with_locked_buffer(*buffer);
      interrupts::enable();
    }
    busy_sleep(10_us);
  }
}
