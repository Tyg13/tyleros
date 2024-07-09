#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

namespace interrupts {

struct interrupt_frame {
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
} __attribute__((packed));
static_assert(sizeof(interrupt_frame) == 40);

uintptr_t get_handler(unsigned int vector_index);

inline void enable() { asm volatile("sti" ::: "memory", "cc"); }
inline void disable() { asm volatile("cli" ::: "memory", "cc"); }
inline bool enabled() {
  uint64_t flags;
  asm volatile("pushfq\n"
               "popq %0\n"
               : "=&g"(flags));
  return (flags & (1 << 9)) != 0;
}

struct scoped_disable {
  scoped_disable() : interrupts_were_enabled{interrupts::enabled()} {
    interrupts::disable();
  }
  ~scoped_disable() {
    if (interrupts_were_enabled)
      interrupts::enable();
  }

private:
  bool interrupts_were_enabled;
};

} // namespace interrupts

#endif
