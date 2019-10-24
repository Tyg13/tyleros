#include "kernel.h"

#include "gdt.h"
#include "pit.h"
#include "memory.h"
#include "idt.h"
#include "sse.h"
#include "vga.h"

#include "stdio.h"

void kprintf(const char * fmt, ...) {
   va_list args;

   va_start(args, fmt);
   auto buff_size = vsprintf(NULL, fmt, args);
   va_end(args);

   char * str = (char *)kmalloc(buff_size * sizeof(char));

   va_start(args, fmt);
   vsprintf(str, fmt, args);
   va_end(args);

   vga::string(str).write();
}

extern volatile unsigned int ticks_elapsed;
void sleep(unsigned int seconds) {
   ticks_elapsed = 0;

   unsigned int ticks_needed = seconds * PIT_BASE_RELOAD_FREQUENCY;
   while (ticks_elapsed < ticks_needed) {
      asm volatile("pause");
   }
}

void kmain(void)
{
   vga::clear_screen();
   enable_sse();
   init_gdt();
   init_interrupts();
   init_memory();
   const auto& idtr = get_current_idtr();
   kprintf("IDTR size: %u\nIDTR base: %x", idtr.limit, idtr.base);
}
