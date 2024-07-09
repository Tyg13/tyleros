#include "scheduler.h"

#include "alloc.h"
#include "cmos.h"
#include "debug.h"
#include "interrupts.h"
#include "memory.h"

#include <assert.h>
#include <stdint.h>

extern "C" scheduler::fxsave_data temp_fxsave;

namespace scheduler {

static task_context *tasks = nullptr;
static fxsave_data *fxsave_blocks = nullptr;
static constexpr auto MAX_TASKS = 0x100;
static constexpr auto KERNEL_TASK_ID = 0;
static unsigned int current_task = 0;
static unsigned int total_tasks = 1;
static constexpr auto TASK_STACK_SIZE = memory::PAGE_SIZE * 4;
static constexpr auto TASK_STACK_ALIGN = kstd::Align{16};

unsigned int get_current_task() { return current_task; }

void init() {
  tasks = alloc::zeroed_array_of<task_context>(MAX_TASKS);
  tasks[KERNEL_TASK_ID].state = task_state::runnable;

  fxsave_blocks = alloc::zeroed_array_of<fxsave_data>(MAX_TASKS);

  init_real_time_clock();
  debug::puts("scheduler: initialized");
}

bool task_switching_enabled = false;
void schedule_task(task *new_task, void *context) {
  task_context *task_slot = nullptr;
  for (unsigned i = 0; i < total_tasks; ++i)
    if (tasks[i].state == task_state::dead)
      task_slot = &tasks[i];

  auto &task = task_slot ? *task_slot : tasks[total_tasks];
  assert(total_tasks < MAX_TASKS && "ran out of tasks!");

  task.state = task_state::runnable;

  // Allocate a stack for the task, and make sure the stack pointer
  // points to the end of the buffer (stack grows downwards)
  char *new_stack_top = (char *)alloc::alloc(TASK_STACK_SIZE, TASK_STACK_ALIGN,
                                             alloc::protection::READ_WRITE);
  char *new_stack_base = new_stack_top + TASK_STACK_SIZE;
  task.stack_base = new_stack_base;

  // Put the address of exit() on the stack, so the task terminates properly
  new_stack_base -= sizeof(uintptr_t);
  *(intptr_t *)new_stack_base = reinterpret_cast<intptr_t>(&exit);

  task.frame = task_frame{};
  task.frame.rsp = reinterpret_cast<uint64_t>(new_stack_base);

  task.frame.rip = reinterpret_cast<uint64_t>(new_task);
  task.frame.cs = 0x8;
  task.frame.ss = 0x10;

  // Enable interrupts
  task.frame.rflags = 1 << 9;

  // System V ABI expects the first argument in rdi
  // Hence, to get the call new_task(context) put context in rdi
  task.frame.rdi = reinterpret_cast<uint64_t>(context);

  // Don't update the total number of tasks until the task is safely
  // enqueued, in case we get pre-empted in the middle of scheduling it
  asm volatile("" ::: "memory");
  ++total_tasks;
}

extern "C" void task_switch(task_frame *tcb) {
  // If interrupts are enabled, we might fire another timer interrupt, which
  // will call back into this function. Avoid that by disabling interrupts
  // while checking for a task switch. This isn't really perfect, because we
  // only need to disable the timer interrupt, not all interrupts, but this
  // will do for now.
  interrupts::scoped_disable disable_interrupts;

  // find the next runnable task in our task list
  const unsigned int old_task = current_task;
  unsigned int next_task = old_task;
  do {
    next_task = next_task >= total_tasks - 1 ? 0 : next_task + 1;
  } while (tasks[next_task].state != task_state::runnable);

  // No need to task switch
  if (next_task == current_task)
    return;

  // save the frame of the old task being switched out
  tasks[old_task].frame = *tcb;
  fxsave_blocks[old_task] = temp_fxsave;

  // load the frame of the next task being switched in
  temp_fxsave = fxsave_blocks[next_task];
  *tcb = tasks[next_task].frame;

  current_task = next_task;
}

void yield() {
  if (interrupts::enabled() && task_switching_enabled)
    asm volatile("int $0x28");
}

void exit() {
  {
    interrupts::scoped_disable disable;

    // kill the task and deallocate its stack
    tasks[current_task].state = task_state::dead;
    alloc::free(tasks[current_task].stack_base - TASK_STACK_SIZE,
                TASK_STACK_ALIGN);
  }

  yield();
loop:
  goto loop;
}

} // namespace scheduler
