#include "alloc.h"

#include "scheduler.h"
#include "util.h"
#include "panic.h"

#include <stdint.h>
#include <platform_specific.h>

extern "C" uint64_t syscall_handler(uint64_t code, uint64_t arg0, uint64_t arg1,
                                    uint64_t arg2,
                                    scheduler::task_frame *frame) {
  switch (code) {
  case _SYSCALL_EXIT:
    scheduler::exit();
    return 0;
  case _SYSCALL_ABORT:
    asm volatile("\txchg %%bx, %%bx; hlt" :::);
    while (true)
      ;
  case _SYSCALL_ALLOC:
    return (uint64_t)alloc::alloc((size_t)arg0, kstd::Align(arg1),
                                  alloc::protection::READ_WRITE);
  case _SYSCALL_FREE:
    alloc::free((void *)arg0);
    return 0;
  case _SYSCALL_PRINT:
    puts((const char *)arg0);
    return 0;
  default:
    kstd::panic("unknown syscall: %lx", code);
  }
}
