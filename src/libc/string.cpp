#include <string.h>

#include <stdint.h>

template <typename T> struct volatile_helper_impl {
  using type = unsigned char;
};
template <typename T> struct volatile_helper_impl<volatile T *> {
  using type = volatile unsigned char;
};
template <typename T>
using volatile_helper = typename volatile_helper_impl<T>::type;

template <typename T>
__attribute__((always_inline)) static inline auto
memcpy_impl(T *dst, const T *src, size_t n) {
  using char_type = volatile_helper<decltype(dst)>;
  auto __dst = reinterpret_cast<char_type *>(dst);
  auto __src = reinterpret_cast<const char_type *>(src);
  for (size_t i = 0; i < n; ++i) {
    *__dst++ = *__src++;
  }
  return dst;
}

template <typename T>
__attribute__((always_inline)) static inline auto memset_impl(T *dst, int c,
                                                              size_t n) {
  using char_type = volatile_helper<decltype(dst)>;
  auto __dst = reinterpret_cast<char_type *>(dst);
  for (size_t i = 0; i < n; ++i) {
    *__dst++ = (char_type)c;
  }
  return dst;
};

volatile void *memcpy_v(volatile void *dst, const volatile void *src,
                        size_t n) {
  return memcpy_impl(dst, src, n);
}

volatile void *memset_v(volatile void *dst, int c, size_t n) {
  return memset_impl(dst, c, n);
}

void *memcpy(void *dst, const void *src, size_t n) {
  return memcpy_impl(dst, src, n);
}

void *memmove(void *dst, const void *src, size_t n) {
  auto *__dst = (char*)dst;
  const auto *__src = (const char*)src;
  // If dst is behind src, copy forward:
  //   dst: xxxxxxxx
  //   src: ^  xxxxxxxxx
  //         \_|   ^__|
  if ((uintptr_t)dst < (uintptr_t)src)
    for (unsigned i = 0; i < n; ++i)
      __dst[i] = __src[i];
  // If src is behind dst, copy backwards:
  //   dst:    xxxxxxxx
  //   src: xxxxxxxxx ^
  //        |__^   |_/
  else
    for (unsigned i = n; i > 0; --i)
      __dst[i - 1] = __src[i - 1];
  return dst;
}

void *memset(void *dst, int c, size_t n) { return memset_impl(dst, c, n); }

char *strchr(const char *str, int character) {
  if (str) {
    for (int i = 0; str[i] != '\0'; ++i) {
      if (str[i] == (char)character) {
        return const_cast<char *>(&str[i]);
      }
    }
  }
  return nullptr;
}

int strncmp(const char *str1, const char *str2, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (str1[i] == '\0' && str2[i] != '\0') {
      return -1;
    }
    if (str2[i] == '\0' && str1[i] != '\0') {
      return 1;
    }
    if (str2[i] == '\0' && str1[i] == '\0') {
      return 0;
    }
    if (str1[i] != str2[i]) {
      return str1[i] < str2[i] ? -1 : 1;
    }
  }
  return 0;
}

int strcmp(const char *str1, const char *str2) {
  return strncmp(str1, str2, SIZE_MAX);
}

char *strncpy(char *dst, const char *src, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    dst[i] = src[i];
    if (src[i] == '\0') {
      break;
    }
  }
  return dst;
}

char *strcpy(char *dst, const char *src) { return strncpy(dst, src, SIZE_MAX); }

size_t strlen(const char *src) {
  size_t len = 0;
  while (*src++ != '\0') {
    ++len;
  }
  return len;
}

char *strrev(char *src, int len) {
  if (src) {
    char *begin = src;
    char *end = src + len - 1;
    while (begin < end) {
      auto tmp = *end;
      *end-- = *begin;
      *begin++ = tmp;
    }
  }
  return src;
}
