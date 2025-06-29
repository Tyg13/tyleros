#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "gdt.h"
#include "interrupts.h"
#include "platform_specific.h"
#include <stdint.h>

namespace scheduler {

struct task_frame;

extern "C" bool task_switching_enabled;
extern "C" bool no_scheduler_tick;
extern "C" void task_switch(task_frame *tcb);

void init();

using task = void(void *);

struct alignas(16) fxsave_data {
  char data[512];
};

struct task_frame {
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;
  uint64_t rbp;
  uint64_t rdi;
  uint64_t rsi;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rbx;
  uint64_t rax;
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
} __attribute__((packed));

enum class task_state {
  killed = 0,
  waiting = 1,
  running = 2,
};

struct task_context {
  task_frame frame;
  task_state state;
  void *stack_base;
};
struct task_id {
  unsigned int id;
  operator unsigned int() const { return id; }
};

task_id schedule_user_task(task *new_task, void *context);
task_id schedule_kernel_task(task *new_task, void *context);

task_id get_current_task_id();
task_context *get_current_task();
inline bool get_current_task_is_kernel() {
  return !get_current_task() ||
         get_current_task()->frame.cs == gdt::KERNEL_CODE_SELECTOR;
}
inline bool get_current_task_is_user() {
  return get_current_task() &&
         get_current_task()->frame.cs == gdt::USER_CODE_SELECTOR;
}
void kill(task_id id);

void task_switch();

__attribute__((always_inline)) inline void yield() {
  if (interrupts::enabled() && task_switching_enabled) {
    no_scheduler_tick = true;
    asm volatile("int $0x28");
  } else
    asm volatile("pause");
}
[[noreturn]] void exit();

inline void enable_task_switch() {
  interrupts::with_interrupts_disabled([]() { task_switching_enabled = true; });
}

} // namespace scheduler

#endif
