#ifndef UTIL_H
#define UTIL_H

#include "debug.h"
#include "memory.h"
#include "util/type_traits.h"
#include "vga.h"

#include <stdio.h>
#include <stdarg.h>

namespace kstd {
   template <typename Element, typename Transform>
   void transform(Element array[], int num_of_elements, Transform transform_element) {
      for (auto i = 0; i < num_of_elements; ++i) {
         using ResultType = decltype(transform_element(array[i]));
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
   void insertion_sort(Element array[], int num_of_elements, Compare cmp) {
      for (auto i = 0; i < num_of_elements; ++i) {
         for (auto j = i; j < num_of_elements; ++j) {
            if (cmp(array[j], array[i])) {
               swap(array[i], array[j]);
            }
         }
      }
   }
}

inline constexpr auto max = [](auto a, auto b) { return a > b ? a : b; };
inline constexpr auto min = [](auto a, auto b) { return a < b ? a : b; };
inline constexpr auto div_round_up = [](auto a, auto b) { return a / b + (a % b != 0); };
inline constexpr auto round_up_to_multiple = [](auto a, auto multiple) {
    return ((a + multiple - 1) / multiple) * multiple;
};

__attribute__((format (printf, 1, 2)))
inline void kprintf(const char * fmt, ...) {
   va_list args;

   va_start(args, fmt);
   const auto buff_size = vsprintf(NULL, fmt, args);
   va_end(args);

   const auto str = new char[buff_size];

   va_start(args, fmt);
   vsprintf(str, fmt, args);
   va_end(args);

   vga::string(str).write();

   delete[] str;
}

inline void vassert(bool value, const char * msg, va_list args) {
   if (value) {
       return;
   }
   if (msg) {
       char buffer[512];
       vsprintf(buffer, msg, args);

       debug::write_str(buffer);
       vga::string(buffer).write();
   }
   asm volatile ("hlt");
loop: goto loop;
}

__attribute__((format (printf, 2, 3)))
inline void assert(bool value, const char * msg, ...) {
   va_list args;
   va_start(args, msg);
   vassert(value, msg, args);
   va_end(args);
}

[[noreturn]]
__attribute__((format (printf, 1, 2)))
inline void panic(const char * msg, ...) {
   va_list args;
   va_start(args, msg);
   vassert(false, msg, args);
   __builtin_unreachable();
}

#endif
