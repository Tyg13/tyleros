#include "interrupts.h"

#include "cmos.h"
#include "pic.h"

#define INTERRUPT __attribute__((interrupt))

INTERRUPT void timer_handler(interrupt_frame* frame);
INTERRUPT void pic1_irq_handler(interrupt_frame* frame);
INTERRUPT void real_time_clock_handler(interrupt_frame* frame);
INTERRUPT void pic2_irq_handler(interrupt_frame* frame);
INTERRUPT void interrupt_handler(interrupt_frame* frame);
INTERRUPT void exception_handler(interrupt_frame* frame, size_t error_code);

#undef INTERRUPT

uintptr_t get_interrupt_handler(unsigned int vector_index) {
   constexpr auto cast = [](const auto & handler) { return reinterpret_cast<uintptr_t>(handler); };
   switch (vector_index) {
      case 0x20:          return cast(timer_handler);
      case 0x22 ... 0x27: return cast(pic1_irq_handler);
      case 0x28:          return cast(real_time_clock_handler);
      case 0x29 ... 0x2F: return cast(pic2_irq_handler);
      default:            return cast(interrupt_handler);
   }
}
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

static unsigned ticks = 0;
void real_time_clock_handler(interrupt_frame * frame) {
   ++ticks;

   // Status register C tells us why the CMOS triggered the interrupt.
   // We don't care why (we know the interrupt came from the RTC)
   // but we still have to read it because the CMOS won't trigger another
   // interrupt until we do so.
   io::out(CMOS_COMMAND, RTC_SELECT_C);
   io::in(CMOS_DATA);

   io::out(PIC2_COMMAND, END_OF_INTERRUPT);
   io::out(PIC1_COMMAND, END_OF_INTERRUPT);
}

void exception_handler(interrupt_frame* frame, size_t error_code) { }

