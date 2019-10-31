#ifndef UTIL_H
#define UTIL_H

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

   template <typename Element>
   void insertion_sort(Element array[], int num_of_elements, bool (*cmp)(const Element&, const Element&)) {
      for (auto i = 0; i < num_of_elements; ++i) {
         for (auto j = i; j < num_of_elements; ++j) {
            if (cmp(array[j], array[i])) {
               swap(array[i], array[j]);
            }
         }
      }
   }
}

inline constexpr auto div_round_up = [](const auto& a, const auto &b) { return a / b + (a % b != 0); };

__attribute__((format (printf, 1, 2)))
inline void kprintf(const char * fmt, ...) {
   va_list args;

   va_start(args, fmt);
   auto buff_size = vsprintf(NULL, fmt, args);
   va_end(args);

   char * str = new char[buff_size];

   va_start(args, fmt);
   vsprintf(str, fmt, args);
   va_end(args);

   vga::string(str).write();

   kfree(str);
}

#endif
