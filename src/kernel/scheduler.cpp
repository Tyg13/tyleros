#include "scheduler.h"

#include "memory.h"
#include "paging.h"

#include <string.h>
#include <stdint.h>

static task_context * tasks = nullptr;
static constexpr auto MAX_TASKS = 0x100;
static unsigned int current_task = 0;
static unsigned int total_tasks  = 1;
static constexpr auto TASK_STACK_SIZE = PAGE_SIZE * 4;

unsigned int get_current_task() {
   return current_task;
}

void init_scheduler() {
   tasks = new task_context[MAX_TASKS];
   memset(tasks, 0, MAX_TASKS * sizeof(task_context));
}

uint8_t should_task_switch = 0;
void schedule_task(task * new_task) {
   auto & task = tasks[total_tasks];

   task.state = task_state::runnable;

   // Allocate a stack for the task, and make sure the stack pointer
   // points to the end of the buffer (stack grows downwards)
   const auto new_rsp = new char[TASK_STACK_SIZE] + TASK_STACK_SIZE;
   task.frame.rsp = reinterpret_cast<uint64_t>(new_rsp);

   task.frame.rip = reinterpret_cast<uint64_t>(new_task);
   task.frame.cs = 0x8;
   task.frame.ss = 0x10;
   task.frame.rflags = 1 << 9;

   // Don't update the total number of tasks until the task is safely
   // enqueued, in case we get pre-empted in the middle of scheduling it
   asm volatile ("":::"memory");
   ++total_tasks;
}

task_frame * task_switch(task_frame * tcb) {
   constexpr auto next_task = []() {
      ++current_task;
      if (current_task >= total_tasks) {
         current_task = 0;
      }
      return current_task;
   };

   tasks[current_task].frame = *tcb;
   while (tasks[next_task()].state != task_state::runnable);
   *tcb = tasks[current_task].frame;
   return tcb;
};

void yield() {
   asm volatile ("int $0x28");
}

void exit() {
   asm volatile ("cli" ::: "memory");
   tasks[current_task].state = task_state::dead;
   asm volatile ("sti" ::: "memory");
   asm volatile ("int $0x28");
   __builtin_unreachable();
}
