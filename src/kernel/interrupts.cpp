#include "interrupts.h"

#include "cmos.h"
#include "pic.h"
#include "scheduler.h"
#include "util.h"
#include "vga.h"

#include <stdio.h>

#define INTERRUPT __attribute__((interrupt))

INTERRUPT void page_fault_handler(interrupt_frame* frame, size_t error_code);
INTERRUPT void timer_handler(interrupt_frame* frame);
INTERRUPT void keyboard_handler(interrupt_frame * frame);
INTERRUPT void pic1_irq_handler(interrupt_frame* frame);
extern "C" task_frame* frame_handler(task_frame* tcb);

extern "C" void scheduler_interrupt();
INTERRUPT void pic2_irq_handler(interrupt_frame* frame);
INTERRUPT void interrupt_handler(interrupt_frame* frame);
INTERRUPT void exception_handler(interrupt_frame* frame, size_t error_code);

#undef INTERRUPT

uintptr_t get_interrupt_handler(unsigned int vector_index) {
   constexpr auto cast = [](const auto & handler) { return reinterpret_cast<uintptr_t>(handler); };
   switch (vector_index) {
      case 0x0E:          return cast(page_fault_handler);
      case 0x20:          return cast(timer_handler);
      case 0x21:          return cast(keyboard_handler);
      case 0x22 ... 0x27: return cast(pic1_irq_handler);
      case 0x28:          return cast(scheduler_interrupt);
      case 0x29 ... 0x2F: return cast(pic2_irq_handler);
      default:            return cast(interrupt_handler);
   }
}

void page_fault_handler(interrupt_frame* frame, size_t error_code) {
   void * fault_address;
   asm volatile ("mov %%cr2, %0" : "=g"(fault_address));
   const auto access_was_read = (error_code ^ (1 << 2)) != 0;
   const auto action = access_was_read ? "reading" : "writing to";
   char message[512];
   sprintf(message, "Page fault occurred %s 0x%p\n", action, fault_address);
   vga::string(message).write();

   sprintf(message,
         "RIP:     0x%lx\n"
         "CS:      0x%lx\n"
         "RFLAGS:  0x%lx\n"
         "RSP:     0x%lx\n"
         "SS:      0x%lx\n"
         "TASK ID: %u\n",
         frame->rip, frame->cs, frame->rflags, frame->rsp, frame->ss, get_current_task());
   vga::string(message).write();

   const auto page_not_present = (error_code ^ 1) != 0;
   if (page_not_present) {
      panic("Page was not present!\n");
   }
}

static constexpr char scancode_to_key[0x40] = {
   '\0', ' ', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
   '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0x0,
   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0x1, '\\',
   'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0x2, '\0', 0x2, ' ',
};

void keyboard_handler(interrupt_frame* frame) {
   constexpr auto PS_2_DATA = 0x60;
   const auto scancode = io::in(PS_2_DATA);
   if (scancode < 0x40) {
      const auto key = scancode_to_key[scancode];
      const char buffer[2] = { key, '\0' };
      vga::string(buffer).write();
   }

   io::out(PIC1_COMMAND, END_OF_INTERRUPT);
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

task_frame* frame_handler(task_frame* tcb) {
   return task_switch(tcb);
}

void exception_handler(interrupt_frame* frame, size_t error_code) { }

