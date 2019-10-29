#include "kernel.h"

#include "gdt.h"
#include "idt.h"
#include "memory.h"
#include "sse.h"
#include "vga.h"

#include "stdio.h"

__attribute__((format (printf, 1, 2)))
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

   kfree(str);
}

void kmain(void)
{
   vga::clear_screen();
   enable_sse();
   init_gdt();
   init_interrupts();
   init_memory();
loop:
   goto loop;
}
