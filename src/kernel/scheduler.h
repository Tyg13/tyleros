#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

namespace scheduler {

void init();

using task = void(void *);

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
  runnable,
  sleeping,
  dead,
};

struct task_context {
  task_frame frame;
  task_state state;
};

void schedule_task(task *new_task, void *context);

task_frame *task_switch(task_frame *tcb);

unsigned int get_current_task();

void yield();
[[noreturn]] void exit();

extern "C" uint8_t should_task_switch;

inline void enable_task_switch() {
  asm volatile("cli" ::: "memory");
  should_task_switch = 1;
  asm volatile("sti" ::: "memory");
}

} // namespace scheduler

#endif
