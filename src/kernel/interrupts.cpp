#include "interrupts.h"

#include "cmos.h"
#include "floppy.h"
#include "pic.h"
#include "scheduler.h"
#include "timing.h"
#include "util.h"
#include "vga.h"

#include <stdio.h>

#define INTERRUPT __attribute__((interrupt))

INTERRUPT void page_fault_handler(interrupt_frame *frame, size_t error_code);
INTERRUPT void keyboard_handler(interrupt_frame *frame);
INTERRUPT void timer_handler(interrupt_frame *frame);
INTERRUPT void floppy_handler(interrupt_frame *frame);
INTERRUPT void interrupt_handler(interrupt_frame *frame);

extern "C" scheduler::task_frame *frame_handler(scheduler::task_frame *tcb);
extern "C" void scheduler_interrupt();

#undef INTERRUPT

uintptr_t get_interrupt_handler(unsigned int index) {
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
  default:
    return CAST(interrupt_handler);
#undef CAST
  }
}

#define SAFE_FN __attribute__((no_caller_saved_registers))

template <typename F, typename... Args>
__attribute__((always_inline)) inline SAFE_FN auto
wrap_unsafe_fn(F &&f, Args &&...args) {
  return f(kstd::forward<Args>(args)...);
}

SAFE_FN static void print_interrupt_frame(interrupt_frame *frame,
                                          char message[512]) {
  snprintf(message, 512,
           "RIP:     0x%lx\n"
           "CS:      0x%lx\n"
           "RFLAGS:  0x%lx\n"
           "RSP:     0x%lx\n"
           "SS:      0x%lx\n"
           "TASK ID: %u\n",
           frame->rip, frame->cs, frame->rflags, frame->rsp, frame->ss,
           scheduler::get_current_task());
  debug::puts(message);
}

void page_fault_handler(interrupt_frame *frame, size_t error_code) {
  void *fault_address;
  asm volatile("mov %%cr2, %0" : "=r"(fault_address));
  const auto access_was_read = (error_code & (1 << 1)) == 0;
  const auto action = access_was_read ? "reading from" : "writing to";
  char message[512];
  wrap_unsafe_fn(snprintf, message, 64, "Page fault occurred %s 0x%p\n", action,
                 fault_address);

  wrap_unsafe_fn(debug::puts, message);

  print_interrupt_frame(frame, message);

  const auto page_not_present = (error_code ^ 1) != 0;
  if (page_not_present) {
    wrap_unsafe_fn(panic, "Page was not present!\n");
  }
}

void timer_handler(interrupt_frame *frame) {
  wrap_unsafe_fn(tick);
  wrap_unsafe_fn(io::out, PIC1_COMMAND, END_OF_INTERRUPT);
}

void floppy_handler(interrupt_frame *frame) {
  wrap_unsafe_fn(io::out, PIC1_COMMAND, END_OF_INTERRUPT);
  disk_interrupt_handled = true;
}

static constexpr char scancode_to_key[0x40] = {
    '\0', ' ', '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-',  '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o',  'p', '[',  ']',  '\n', 0x0, 'a', 's',  'd', 'f', 'g', 'h',
    'j',  'k', 'l',  ';',  '\'', '`', 0x1, '\\', 'z', 'x', 'c', 'v',
    'b',  'n', 'm',  ',',  '.',  '/', 0x2, '\0', 0x2, ' ',
};

void keyboard_handler(interrupt_frame *frame) {
  constexpr auto PS_2_DATA = 0x60;
  const auto scancode = wrap_unsafe_fn(io::in, PS_2_DATA);
  if (scancode < 0x40) {
    const auto key = scancode_to_key[scancode];
    const char buffer[2] = {key, '\0'};
    wrap_unsafe_fn(vga::string::puts, buffer);
  }

  wrap_unsafe_fn(io::out, PIC1_COMMAND, END_OF_INTERRUPT);
}

void interrupt_handler(interrupt_frame *frame) {
  char message[512];
  print_interrupt_frame(frame, message);
  wrap_unsafe_fn(panic, "Unhandled unknown interrupt!");
}

scheduler::task_frame *frame_handler(scheduler::task_frame *tcb) {
  return task_switch(tcb);
}
