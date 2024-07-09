#ifndef UTIL_H
#define UTIL_H
#include "algorithm.h"

#include <bit>
#include <cpuid.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

namespace kstd {
struct Align {
  using repr_type = decltype(alignof(char));

  constexpr Align() = default;
  constexpr explicit Align(repr_type val) : val{val} {}

  repr_type val = 1;
};

inline constexpr uint64_t align_to(uint64_t val, Align align) {
  return (val + align.val - 1) / align.val * align.val;
}

template <typename T> constexpr inline Align align_of = Align{alignof(T)};

template <typename T> constexpr T div_ceil(const T &a, const T &b) {
  return a / b + (a % b != 0);
}

template <typename T> constexpr int log2_floor(T v) {
  return std::bit_width(v) - 1;
}

#define SPIN_WHILE(x)                                                          \
  do {                                                                         \
    asm volatile("pause");                                                     \
  } while (x)
#define SPIN_UNTIL(x) SPIN_WHILE(!(x))

[[noreturn]] inline void panic(const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  asm volatile("hlt");
loop:
  goto loop;
}
} // namespace kstd

struct cpuid_info {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
};

inline cpuid_info cpuid(uint32_t code) {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
  __cpuid(code, eax, ebx, ecx, edx);
  return cpuid_info{eax, ebx, ecx, edx};
}

#endif
