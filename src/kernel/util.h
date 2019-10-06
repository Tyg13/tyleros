#ifndef UTIL_H
#define UTIL_H

namespace std {
   template <typename T> struct remove_reference      { using type = T; };
   template <typename T> struct remove_reference<T&>  { using type = T; };
   template <typename T> struct remove_reference<T&&> { using type = T; };
   template <typename T> using remove_reference_t = typename remove_reference<T>::type;

   template <typename T>
   constexpr inline auto move(T&& t) { return static_cast<remove_reference_t<T>&&>(t); }
}

template <typename T>
void swap(T& lhs, T& rhs) {
    auto tmp = std::move(lhs);
    lhs = std::move(rhs);
    rhs = std::move(tmp);
}

#endif
