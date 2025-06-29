#include "interrupts.h"

#include "debug.h"
#include "floppy.h"
#include "gdt.h"
#include "input.h"
#include "paging.h"
#include "pic.h"
#include "scheduler.h"
#include "timing.h"
#include "panic.h"

#include <assert.h>
#include <stdio.h>
#include <utility>

namespace interrupts {

__attribute__((interrupt)) void
general_protection_fault_handler(interrupt_frame *frame, size_t error_code);
__attribute__((interrupt)) void page_fault_handler(interrupt_frame *frame,
                                                   size_t error_code);
__attribute__((interrupt)) void keyboard_handler(interrupt_frame *frame);
__attribute__((interrupt)) void timer_handler(interrupt_frame *frame);
__attribute__((interrupt)) void floppy_handler(interrupt_frame *frame);

template <unsigned char N>
__attribute__((interrupt)) void interrupt_handler(interrupt_frame *frame);

extern "C" __attribute__((interrupt)) void
scheduler_interrupt(interrupt_frame *);
extern "C" __attribute__((interrupt)) void
syscall_interrupt(interrupt_frame *);

#undef INTERRUPT

uintptr_t get_handler(unsigned char index) {
#define CAST(handler) reinterpret_cast<uintptr_t>(handler)
  switch (index) {
  case 0x0D:
    return CAST(general_protection_fault_handler);
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
  case 0x80:
    return CAST(syscall_interrupt);
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
  default:
    return CAST(interrupt_handler<static_cast<unsigned char>(-1)>);
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

static void print_interrupt_frame(char buffer[512], interrupt_frame *frame) {
  snprintf(buffer, 512,
           "RIP:     0x%lx\n"
           "CS:      0x%lx\n"
           "RFLAGS:  0x%lx\n"
           "RSP:     0x%lx\n"
           "SS:      0x%lx\n"
           "TASK ID: %u (%s)",
           frame->rip, frame->cs, frame->rflags, frame->rsp, frame->ss,
           (unsigned int)scheduler::get_current_task_id(),
           scheduler::get_current_task_is_kernel() ? "kernel" : "user");
  buffer[511] = '\0';
  puts(buffer);
}

struct selector_error_code {
  size_t v;
  bool is_external() const { return v & 1; }
  unsigned get_table() const { return (v >> 1) & 0b11; }
  unsigned get_index() const { return (v >> 3) & 0b1'1111'1111'1111; }
  unsigned get_selector() const {
    return get_table() == 00 ? get_index() * sizeof(gdt::Entry) : 0;
  }
};

SAFE_FN void general_protection_fault_handler_impl(interrupt_frame *frame,
                                                   size_t error_code) {
  char buffer[512];
  print_interrupt_frame(buffer, frame);
  selector_error_code selector_ec{error_code};
  sprintf(buffer,
          "General protection (GP) fault occurred at %p (tbl=%s, idx=%x)",
          (void *)frame->rip,
          selector_ec.get_table() == 0b00   ? "gdt"
          : selector_ec.get_table() == 0b10 ? "ldt"
                                            : "idt",
          selector_ec.get_index());
  puts(buffer);

  if (selector_ec.get_selector() == gdt::USER_CODE_SELECTOR) {
    assert(scheduler::get_current_task_is_user() && "current task is not user?");
    scheduler::kill(scheduler::get_current_task_id());
    interrupts::enable();
    scheduler::yield();
  } else if (selector_ec.get_selector() == gdt::KERNEL_CODE_SELECTOR) {
  loop:
    goto loop;
  } else {
    kstd::panic("unknown task type!");
  }
}

void general_protection_fault_handler(interrupt_frame *frame, size_t error_code) {
  general_protection_fault_handler_impl(frame, error_code);
}

SAFE_FN
void page_fault_handler_impl(interrupt_frame *frame, size_t error_code) {
  void *fault_address;
  asm volatile("mov %%cr2, %0" : "=r"(fault_address));
  const auto access_was_read = (error_code & (1 << 1)) == 0;
  const auto action = access_was_read ? "reading from" : "writing to";
  char buffer[512];
  snprintf(buffer, 64, "Page fault occurred %s 0x%p", action, fault_address);
  puts(buffer);

  print_interrupt_frame(buffer, frame);

  const auto page_not_present = (error_code ^ 1) != 0;
  if (page_not_present)
    puts("Page was not present!");

  asm volatile("hlt");
loop:
  goto loop;
}

void page_fault_handler(interrupt_frame *frame, size_t error_code) {
  page_fault_handler_impl(frame, error_code);
}

void timer_handler(interrupt_frame *frame) {
  wrap_unsafe_fn([]() {
    tick();
    pic::signal_end_of_interrupt(irq::PIT);
  });
}

void floppy_handler(interrupt_frame *frame) {
  wrap_unsafe_fn([]() {
    handle_floppy_interrupt();
    pic::signal_end_of_interrupt(irq::FLOPPY);
  });
}

void keyboard_handler(interrupt_frame *frame) {
  wrap_unsafe_fn([]() {
    keyboard::handle_interrupt();
    pic::signal_end_of_interrupt(irq::KEYBOARD);
  });
}

template <unsigned char N> void interrupt_handler(interrupt_frame *frame) {
  wrap_unsafe_fn([=]() {
    char buffer[512];
    print_interrupt_frame(buffer, frame);
    switch (N) {
    case 0:
      kstd::panic("Divide by zero!");
    default:
      kstd::panic("Unhandled unknown interrupt: 0x%x!", N);
    }
  });
}
} // namespace interrupts
