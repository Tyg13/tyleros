#ifndef UTIL_H
#define UTIL_H

namespace kstd {
   template <typename T> struct remove_reference      { using type = T; };
   template <typename T> struct remove_reference<T&>  { using type = T; };
   template <typename T> struct remove_reference<T&&> { using type = T; };
   template <typename T> using remove_reference_t = typename remove_reference<T>::type;

   template <typename T>
   constexpr inline remove_reference_t<T> && move(T&& t) noexcept
   { return static_cast<remove_reference_t<T>&&>(t); }

   template <typename T>
   constexpr inline T && forward(remove_reference_t<T>&& t) noexcept
   { return static_cast<T&&>(t); }

   template <typename T>
   constexpr inline T && forward(remove_reference_t<T>& t)  noexcept
   { return static_cast<T&&>(t); }

   template <typename T>
   void swap(T& lhs, T& rhs) {
       auto tmp = kstd::move(lhs);
       lhs = kstd::move(rhs);
       rhs = kstd::move(tmp);
   }

   template <typename T, typename U> struct is_same       { constexpr static bool value = false; };
   template <typename T>             struct is_same<T, T> { constexpr static bool value = true;  };
   template <typename T, typename U> constexpr static inline bool is_same_v = is_same<T, U>::value;

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
