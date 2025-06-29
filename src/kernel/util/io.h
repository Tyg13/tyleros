#ifndef UTIL_IO_H
#define UTIL_IO_H

#include <stdint.h>

namespace io {
enum mode {
  read,
  write,
  readwrite,
};

template <mode pty> struct port {
  uint16_t val;
};

__attribute__((used))
__attribute__((__always_inline__)) static inline void outb(uint16_t port,
                                                           uint8_t val) {
  asm volatile("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

__attribute__((used))
__attribute__((__always_inline__)) static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
  return ret;
}

__attribute__((used))
__attribute__((__always_inline__)) static inline void outw(uint16_t port,
                                                           uint16_t val) {
  asm volatile("outw %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

__attribute__((__always_inline__)) static inline void outb(port<write> port,
                                                           uint8_t val) {
  outb(port.val, val);
}

__attribute__((__always_inline__)) static inline void outw(port<readwrite> port,
                                                           uint16_t val) {
  outw(port.val, val);
}

__attribute__((__always_inline__)) static inline void outw(port<write> port,
                                                           uint16_t val) {
  outw(port.val, val);
}

__attribute__((__always_inline__)) static inline void outb(port<readwrite> port,
                                                           uint8_t val) {
  outb(port.val, val);
}

__attribute__((__always_inline__)) static inline uint8_t inb(port<read> port) {
  return inb(port.val);
}

__attribute__((__always_inline__)) static inline uint8_t
inb(port<readwrite> port) {
  return inb(port.val);
}

__attribute__((__always_inline__)) static inline void wait() {
  // Port 0x80 is a debug port used by the BIOS during POST.
  // Should be safe for us to use to wait for out/in calls to properly finish.
  outb(0x80, 0);
}
} // namespace io

#endif
