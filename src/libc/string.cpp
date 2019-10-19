#include <string.h>

template <typename T> struct volatile_helper_impl               { using type = unsigned char; };
template <typename T> struct volatile_helper_impl<volatile T *> { using type = volatile unsigned char; };
template <typename T> using volatile_helper = typename volatile_helper_impl<T>::type;

template <typename T>
__attribute__((always_inline))
static inline auto memcpy_impl(T * dst, const T * src, size_t n) {
   using char_type = volatile_helper<decltype(dst)>;
   auto __dst = reinterpret_cast<char_type *>(dst);
   auto __src = reinterpret_cast<const char_type *>(src);
   for (size_t i = 0; i < n; ++i) {
      *__dst++ = *__src++;
   }
   return __dst;
}

template <typename T>
__attribute__((always_inline))
static inline auto memset_impl(T * dst, int c, size_t n) {
   using char_type = volatile_helper<decltype(dst)>;
   auto __dst = reinterpret_cast<char_type *>(dst);
   for (size_t i = 0; i < n; ++i) {
      *__dst++ = (char_type) c;
   }
   return __dst;
};

void * memcpy(void * dst, const void * src, size_t n) {
   return memcpy_impl(dst, src, n);
}

volatile void * memcpy_v(volatile void * dst, const volatile void * src, size_t n) {
   return memcpy_impl(dst, src, n);
}

void * memset(void * dst, int c, size_t n) {
   return memset_impl(dst, c, n);
}

volatile void * memset_v(volatile void * dst, int c, size_t n) {
   return memset_impl(dst, c, n);
}

size_t strlen(const char * src) {
   size_t len = 0;
   while (*src++ != '\0') {
      ++len;
   }
   return len;
}

char * strcpy(char * dst, const char * src) {
   for (int i = 0; ; ++i) {
      *dst = *src;
      if (src[i] == '\0') {
         break;
      }
   }
   return dst;
}

char * strrev(char * src, int len) {
   // len - 1 so we don't reverse the null separator
   for (int i = len - 1; i > 0; --i) {
      char tmp = src[i];
      src[i] = src[len - i];
      src[len - i] = tmp;
   }
   return src;
}
