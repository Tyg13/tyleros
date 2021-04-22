#ifndef UTIL_TYPE_TRAITS_H
#define UTIL_TYPE_TRAITS_H

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


   namespace detail {
      template <class T> struct type_identity { using type = T; };
       
      template <class T> auto try_add_lvalue_reference(int) -> type_identity<T&>;
      template <class T> auto try_add_lvalue_reference(...) -> type_identity<T>;
       
      template <class T> auto try_add_rvalue_reference(int) -> type_identity<T&&>;
      template <class T> auto try_add_rvalue_reference(...) -> type_identity<T>;
   }
   template <class T> struct add_lvalue_reference : decltype(detail::try_add_lvalue_reference<T>(0)) {};
   template <class T> struct add_rvalue_reference : decltype(detail::try_add_rvalue_reference<T>(0)) {};

   template <typename T> using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;
   template <typename T> using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

   template <typename T> auto declval() noexcept -> add_rvalue_reference_t<T>;

   template <typename T>
   void swap(T&& lhs, T&& rhs) {
       auto tmp = kstd::move(lhs);
       lhs = kstd::move(rhs);
       rhs = kstd::move(tmp);
   }

   template <typename T, typename U> struct is_same       { constexpr static bool value = false; };
   template <typename T>             struct is_same<T, T> { constexpr static bool value = true;  };
   template <typename T, typename U> constexpr static inline bool is_same_v = is_same<T, U>::value;

   template <typename T> struct identity { using type = T; };
   template <typename T> using identity_t = typename identity<T>::type;

   template <typename T> using dont_deduce = identity_t<T>;

   template<bool B, class T = void> struct enable_if {};
   template<class T> struct enable_if<true, T> { typedef T type; };
   template<bool B, class T = void> using enable_if_t = typename enable_if<B, T>::type;
}

#endif
