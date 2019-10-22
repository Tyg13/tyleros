#ifndef UTIL_H
#define UTIL_H

#include "util/type_traits.h"

namespace kstd {
   template <typename Element, typename ResultType>
   void transform(Element array[], int num_of_elements, ResultType (*transform_element)(Element&)) {
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

#endif
