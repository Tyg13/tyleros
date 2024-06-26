#ifndef UTIL_H
#define UTIL_H
#include "debug.h"
#include "memory.h"
#include "util/type_traits.h"
#include "vga.h"

#include <stdarg.h>
#include <stdio.h>

namespace kstd {
template <typename Element, typename Transform>
void transform(Element array[], int num_of_elements,
               Transform transform_element) {
  using ResultType = decltype(transform_element(array[0]));
  for (auto i = 0; i < num_of_elements; ++i) {
    if constexpr (is_same_v<ResultType, bool>) {
      if (transform_element(array[i])) {
        return;
      }
    } else {
      transform_element(array[i]);
    }
  }
}

template <typename Element, typename Compare>
void insertion_sort(Element a[], int n, Compare less) {
  for (auto i = 1; i < n; ++i) {
    Element v = a[i];
    auto j = i - 1;
    for (; j >= 0 && !less(a[j], v); --j) {
      a[j + 1] = a[j];
    }
    a[j + 1] = v;
  }
}
} // namespace kstd

inline constexpr auto max = [](auto a, auto b) { return a > b ? a : b; };
inline constexpr auto min = [](auto a, auto b) { return a < b ? a : b; };
inline constexpr auto div_round_up = [](auto a, auto b) {
  return a / b + (a % b != 0);
};
inline constexpr auto round_up_to_multiple = [](auto a, auto multiple) {
  return ((a + multiple - 1) / multiple) * multiple;
};

__attribute__((format(printf, 1, 2))) inline void kprintf(const char *fmt,
                                                          ...) {
  va_list args;

  va_start(args, fmt);
  const auto buff_size = vsprintf(NULL, fmt, args);
  va_end(args);

  const auto str = new char[buff_size];

  va_start(args, fmt);
  vsprintf(str, fmt, args);
  va_end(args);

  vga::string::puts(str);

  delete[] str;
}

inline void vassert(bool value, const char *msg, va_list args) {
  if (value) {
    return;
  }
  if (msg) {
    char buffer[512];
    vsprintf(buffer, msg, args);

    debug::puts(buffer);
    if (vga::initialized) {
      vga::string::puts(buffer);
    }
  }
  asm volatile("hlt");
loop:
  goto loop;
}

__attribute__((format(printf, 2, 3))) inline void assert(bool value,
                                                         const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vassert(value, msg, args);
  va_end(args);
}

[[noreturn]] __attribute__((format(printf, 1, 2))) inline void
panic(const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vassert(false, msg, args);
  __builtin_unreachable();
}

#define SPIN_WHILE(x)                                                          \
  while (x) {                                                                  \
    asm volatile("pause");                                                     \
  }
#define SPIN_UNTIL(x) SPIN_WHILE(!(x))

#endif
