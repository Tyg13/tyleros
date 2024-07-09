#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "interrupts.h"
#include <stdint.h>
#include <utility>

namespace scheduler {

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
  runnable = 0,
  dead,
};

struct task_context {
  task_frame frame;
  task_state state;
  uint8_t code;
  char *stack_base;
};

void schedule_task(task *new_task, void *context);
template <typename F> void schedule_task(F &&f, void *context = nullptr) {
  schedule_task(+std::forward<F>(f), context);
}

unsigned int get_current_task();

void yield();
[[noreturn]] void exit();

extern "C" bool task_switching_enabled;

inline void enable_task_switch() {
  interrupts::disable();
  task_switching_enabled = true;
  interrupts::enable();
}

} // namespace scheduler

#endif
