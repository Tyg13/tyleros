#include "interrupts.h"

#include "pic.h"

uintptr_t get_interrupt_handler(unsigned int i) {
   const auto cast = [](auto && handler) { return reinterpret_cast<uintptr_t>(handler); };
   switch (i) {
      case 0x20:
         return cast(timer_handler);
      case 0x21 ... 0x27:
         return cast(pic1_irq_handler);
      case 0x28 ... 0x2F:
         return cast(pic2_irq_handler);
      default:
         return cast(interrupt_handler);
   }
}

unsigned int ticks_elapsed = 0;
void timer_handler(interrupt_frame* frame) {
   ++ticks_elapsed;
   io::out(PIC1_COMMAND, END_OF_INTERRUPT);
}

void pic1_irq_handler(interrupt_frame* frame) {
   io::out(PIC1_COMMAND, END_OF_INTERRUPT);
}

void pic2_irq_handler(interrupt_frame* frame) {
   io::out(PIC2_COMMAND, END_OF_INTERRUPT);
   io::out(PIC1_COMMAND, END_OF_INTERRUPT);
}

void interrupt_handler(interrupt_frame* frame) { }

void exception_handler(interrupt_frame* frame, size_t error_code) { }

