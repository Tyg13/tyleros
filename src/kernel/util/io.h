#ifndef UTIL_IO_H
#define UTIL_IO_H

#include <stdint.h>

#ifdef __clang__
#define __inline__
#else
#define __inline__ inline
#endif

namespace io {
__attribute__((__always_inline__)) static inline void outb(uint16_t port,
                                                           uint8_t val) {
  asm volatile __inline__("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

__attribute__((__always_inline__)) static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile __inline__("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
  return ret;
}

__attribute__((__always_inline__)) static inline void wait() {
  // Port 0x80 is a debug port used by the BIOS during POST.
  // Should be safe for us to use to wait for out/in calls to properly finish.
  outb(0x80, 0);
}
} // namespace io

#endif
