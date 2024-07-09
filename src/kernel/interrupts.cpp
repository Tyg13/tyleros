#include "interrupts.h"

#include "debug.h"
#include "floppy.h"
#include "input.h"
#include "pic.h"
#include "scheduler.h"
#include "timing.h"
#include "util.h"

#include <stdio.h>
#include <utility>

namespace interrupts {

__attribute__((interrupt)) void page_fault_handler(interrupt_frame *frame,
                                                   size_t error_code);
__attribute__((interrupt)) void keyboard_handler(interrupt_frame *frame);
__attribute__((interrupt)) void timer_handler(interrupt_frame *frame);
__attribute__((interrupt)) void floppy_handler(interrupt_frame *frame);

template <int N>
__attribute__((interrupt)) void interrupt_handler(interrupt_frame *frame);

extern "C" __attribute__((interrupt)) void
scheduler_interrupt(interrupt_frame *);

#undef INTERRUPT

uintptr_t get_handler(unsigned int index) {
#define CAST(handler) reinterpret_cast<uintptr_t>(handler)
  switch (index) {
  case 0x0E:
    return CAST(page_fault_handler);
  case 0x20:
    return CAST(timer_handler);
  case 0x21:
    return CAST(keyboard_handler);
  case 0x26:
    return CAST(floppy_handler);
  case 0x28:
    return CAST(scheduler_interrupt);
#define UNHANDLED(n) case n: return CAST(interrupt_handler<n>);
  UNHANDLED(0x0);
  UNHANDLED(0x1);
  UNHANDLED(0x2);
  UNHANDLED(0x3);
  UNHANDLED(0x4);
  UNHANDLED(0x5);
  UNHANDLED(0x6);
  UNHANDLED(0x7);
  UNHANDLED(0x8);
  UNHANDLED(0x9);
  UNHANDLED(0xa);
  UNHANDLED(0xb);
  UNHANDLED(0xc);
  UNHANDLED(0xd);
  UNHANDLED(0xf);
  UNHANDLED(0x10);
  UNHANDLED(0x11);
  UNHANDLED(0x12);
  UNHANDLED(0x13);
  UNHANDLED(0x14);
  UNHANDLED(0x15);
  UNHANDLED(0x16);
  UNHANDLED(0x17);
  UNHANDLED(0x18);
  UNHANDLED(0x19);
  UNHANDLED(0x1a);
  UNHANDLED(0x1b);
  UNHANDLED(0x1c);
  UNHANDLED(0x1d);
  UNHANDLED(0x1e);
  UNHANDLED(0x1f);
  UNHANDLED(0x22);
  UNHANDLED(0x23);
  UNHANDLED(0x24);
  UNHANDLED(0x25);
  UNHANDLED(0x27);
  default: return CAST(interrupt_handler<-1>);
#undef UNHANDLED
#undef CAST
  }
}

#define SAFE_FN __attribute__((no_caller_saved_registers))

template <typename F, typename... Args>
__attribute__((always_inline)) inline SAFE_FN auto
wrap_unsafe_fn(F &&f, Args &&...args) {
  return f(std::forward<Args>(args)...);
}

SAFE_FN static void print_interrupt_frame(interrupt_frame *frame,
                                          char message[512]) {
  snprintf(message, 512,
           "RIP:     0x%lx\n"
           "CS:      0x%lx\n"
           "RFLAGS:  0x%lx\n"
           "RSP:     0x%lx\n"
           "SS:      0x%lx\n"
           "TASK ID: %u",
           frame->rip, frame->cs, frame->rflags, frame->rsp, frame->ss,
           scheduler::get_current_task());
  debug::puts(message);
}

SAFE_FN
void page_fault_handler_impl(interrupt_frame *frame, size_t error_code) {
  void *fault_address;
  asm volatile("mov %%cr2, %0" : "=r"(fault_address));
  const auto access_was_read = (error_code & (1 << 1)) == 0;
  const auto action = access_was_read ? "reading from" : "writing to";
  char message[512];
  snprintf(message, 64, "Page fault occurred %s 0x%p", action, fault_address);

  debug::puts(message);

  print_interrupt_frame(frame, message);

  const auto page_not_present = (error_code ^ 1) != 0;
  if (page_not_present) {
    debug::puts("Page was not present!");
  }
  asm volatile("hlt");
loop:
  goto loop;
}

void page_fault_handler(interrupt_frame *frame, size_t error_code) {
  page_fault_handler_impl(frame, error_code);
}

void timer_handler(interrupt_frame *frame) {
  wrap_unsafe_fn(tick);
  wrap_unsafe_fn(io::outb, PIC1_COMMAND, END_OF_INTERRUPT);
}

void floppy_handler(interrupt_frame *frame) {
  wrap_unsafe_fn(io::outb, PIC1_COMMAND, END_OF_INTERRUPT);
  disk_interrupt_handled = true;
}

void keyboard_handler(interrupt_frame *frame) {
  wrap_unsafe_fn(keyboard::handle_interrupt);
  wrap_unsafe_fn(io::outb, PIC1_COMMAND, END_OF_INTERRUPT);
}

template <int N> void interrupt_handler(interrupt_frame *frame) {
  char message[512];
  print_interrupt_frame(frame, message);
  switch (N) {
    case 0: wrap_unsafe_fn(kstd::panic, "Divide by zero!");
    case 3: asm volatile ("\txchg %%bx, %%bx; hlt; jmp -0" :::);
    default: wrap_unsafe_fn(kstd::panic, "Unhandled unknown interrupt: 0x%x!", N);
  }
}
} // namespace interrupts
