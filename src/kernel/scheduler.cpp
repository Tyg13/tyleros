#include "scheduler.h"

#include "alloc.h"
#include "cmos.h"
#include "interrupts.h"
#include "memory.h"

#include <assert.h>
#include <stdint.h>

extern "C" scheduler::fxsave_data temp_fxsave;

namespace scheduler {

static constexpr auto MAX_TASKS = 0x100;
static constexpr auto KERNEL_TASK_ID = task_id{0};
static task_id current_task_id;
static unsigned int total_tasks = 1;
static task_context (*tasks)[MAX_TASKS] = nullptr;
static fxsave_data *fxsave_blocks = nullptr;
static constexpr auto TASK_STACK_SIZE = memory::PAGE_SIZE * 4;
static constexpr auto TASK_STACK_ALIGN = kstd::Align{16};

task_id get_current_task_id() { return current_task_id; }
task_context *get_current_task() {
  return tasks ? &(*tasks)[current_task_id] : nullptr;
}

void init() {
  assert(cmos::initialized() && "scheduler requires CMOS!");

  tasks = reinterpret_cast<task_context(*)[MAX_TASKS]>(
      alloc::zeroed_array_of<task_context>(MAX_TASKS));
  current_task_id = KERNEL_TASK_ID;
  (*tasks)[KERNEL_TASK_ID].state = task_state::running;

  fxsave_blocks = alloc::zeroed_array_of<fxsave_data>(MAX_TASKS);

  puts("scheduler: initialized");
}

bool task_switching_enabled = false;
bool no_scheduler_tick = false;
static task_id schedule_task(bool is_kernel, task *new_task, void *context) {
  // Disable interrupts while task switching
  interrupts::scoped_disable disable_interrupts;

  unsigned next_task_id = total_tasks;
  for (unsigned i = 0; i < total_tasks; ++i)
    if ((*tasks)[i].state == task_state::killed) {
      next_task_id = i;
      break;
    }

  assert(next_task_id < MAX_TASKS && "ran out of tasks!");
  if (next_task_id == total_tasks)
    total_tasks += 1;

  task_context &task = (*tasks)[next_task_id];
  task.state = task_state::waiting;

  // Allocate a stack for the task, and make sure the stack pointer
  // points to the end of the buffer (stack grows downwards)
  char *new_stack_top = (char *)alloc::alloc(TASK_STACK_SIZE, TASK_STACK_ALIGN,
                                             alloc::protection::READ_WRITE);
  char *new_stack_base = new_stack_top + TASK_STACK_SIZE;
  task.stack_base = reinterpret_cast<void*>(new_stack_base);

  // Put the address of exit() on the stack, so the task terminates properly
  new_stack_base -= sizeof(uintptr_t);
  *(intptr_t *)new_stack_base = reinterpret_cast<intptr_t>(&exit);

  task.frame = task_frame{};
  task.frame.rsp = reinterpret_cast<uint64_t>(new_stack_base);

  task.frame.rip = reinterpret_cast<uint64_t>(new_task);
  task.frame.cs =
      is_kernel ? gdt::KERNEL_CODE_SELECTOR : gdt::USER_CODE_SELECTOR;
  task.frame.ss =
      is_kernel ? gdt::KERNEL_DATA_SELECTOR : gdt::USER_DATA_SELECTOR;

  // Enable interrupts
  task.frame.rflags = 1 << 9;

  // System V ABI expects the first argument in rdi
  // Hence, to get the call new_task(context) put context in rdi
  task.frame.rdi = reinterpret_cast<uint64_t>(context);

  return task_id{next_task_id};
}

task_id schedule_kernel_task(task *new_task, void *context) {
  return schedule_task(/*is_kernel=*/true, new_task, context);
}
task_id schedule_user_task(task *new_task, void *context) {
  return schedule_task(/*is_kernel=*/false, new_task, context);
}

extern "C" void task_switch(task_frame *tcb) {
  interrupts::scoped_disable disable;

  assert((*tasks)[current_task_id].state == task_state::running &&
         "current task is not running?");

  // find the next runnable task in our task list
  const unsigned int old_task = current_task_id;
  unsigned int next_task = old_task;
  do {
    if (++next_task >= total_tasks)
      next_task = 0;

    // No need to task switch
    if (next_task == current_task_id)
      return;

  } while ((*tasks)[next_task].state != task_state::waiting);

  // save the frame of the old task being switched out
  (*tasks)[old_task].frame = *tcb;
  fxsave_blocks[old_task] = temp_fxsave;

  // load the frame of the next task being switched in
  temp_fxsave = fxsave_blocks[next_task];
  *tcb = (*tasks)[next_task].frame;

  (*tasks)[next_task].state = task_state::running;

  current_task_id.id = next_task;
}

void kill(task_id id) {
  interrupts::with_interrupts_disabled([=]() {
    // kill the task and deallocate its stack
    (*tasks)[id].state = task_state::killed;
    auto *stack_base = reinterpret_cast<char *>((*tasks)[id].stack_base);
    if (stack_base)
      alloc::free(stack_base - TASK_STACK_SIZE);
  });
}

void exit() {
  kill(current_task_id);
  interrupts::enable();
  yield();
loop:
  goto loop;
}

} // namespace scheduler
