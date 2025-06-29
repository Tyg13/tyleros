#include "platform_specific.h"
#include <limits.h>
#include <stddef.h>
#include <string.h>

#include <stdint.h>
#include <stdio.h>

#if TESTING_LIBC
extern "C++" {
namespace kstd {
#endif

using simd_type = unsigned long long;
constexpr static simd_type multiplier_for_broadcast() {
  simd_type multiplier = 0x1;
  for (unsigned i = 0; i < sizeof(simd_type); ++i)
    multiplier = (multiplier << 8) | 0x1;
  return multiplier;
}
static simd_type broadcast(unsigned char c) {
  constexpr static simd_type multiplier = multiplier_for_broadcast();
  return simd_type{c} * multiplier;
}

void *memset(void *dst, int c, size_t n) {
  const auto _c = static_cast<unsigned char>(c);
  auto *__dst = static_cast<unsigned char *>(dst);

  size_t i = 0;

  // first do wide writes, if possible
  if (n >= sizeof(simd_type)) {
    const auto wide_c = broadcast(_c);
    for (; i < n; i += sizeof(simd_type))
      *(simd_type *)(&__dst[i]) = wide_c;
  }

  // handle any remainder
  for (; i < n; ++i)
    __dst[i] = _c;

  return dst;
}

void *memcpy(void *LIBC_RESTRICT dst, const void *LIBC_RESTRICT src, size_t n) {
  auto *__dst = static_cast<unsigned char *>(dst);
  const auto *__src = static_cast<const unsigned char *>(src);
  // first do wide copies
  size_t i = 0;
  for (; i < n / sizeof(simd_type); ++i)
    ((simd_type *)__dst)[i] = ((simd_type *)__src)[i];
  // handle any remainder
  for (size_t j = 0; j < n % sizeof(simd_type); ++j)
    __dst[i * sizeof(simd_type) + j] = __src[i * sizeof(simd_type) + j];
  return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
  auto *__dst = static_cast<unsigned char *>(dst);
  const auto *__src = static_cast<const unsigned char *>(src);
  // If dst is behind src, copy forward:
  //   dst: xxxxxxxx
  //   src: ^  xxxxxxxxx
  //         \_|   ^__|
  if ((uintptr_t)dst < (uintptr_t)src) {
    return memcpy(dst, src, n);
  }
  // If src is behind dst, copy backwards:
  //   dst:    xxxxxxxx
  //   src: xxxxxxxxx ^
  //        |__^   |_/
  else {
    // handle any remainder (in reverse)
    for (size_t i = 0; i < n % sizeof(simd_type); ++i)
      __dst[n - i - 1] = __src[n - i - 1];
    // then do wide copies
    for (size_t i = n / sizeof(simd_type); i > 0; --i)
      ((simd_type *)__dst)[i - 1] = ((simd_type *)__src)[i - 1];
  }
  return dst;
}

void *memchr(const void *ptr, int ch, size_t count) {
  auto c = static_cast<unsigned char>(ch);
  auto *hay = static_cast<unsigned char *>(const_cast<void *>(ptr));
  for (size_t i = 0; i < count; ++i)
    if (hay[i] == c)
      return &hay[i];
  return nullptr;
}

int memcmp(const void *lhs, const void *rhs, size_t count) {
  const auto *a = static_cast<const unsigned char *>(lhs);
  const auto *b = static_cast<const unsigned char *>(rhs);
  for (size_t i = 0; i < count; ++i)
    if (a[i] < b[i])
      return -1;
    else if (a[i] > b[i])
      return 1;
  return 0;
}

char *memstr(const void *src, const char *needle, size_t count) {
  const char *hay = static_cast<const char *>(src);
  const char *hay_end = hay + count;
  const size_t len = strlen(needle);
  while (hay + len < hay_end) {
    char *start = (char *)memchr(hay, needle[0], count);
    if (start == nullptr)
      break;

    if (memcmp(start, needle, len) == 0)
      return start;

    hay = start + 1;
  }
  return nullptr;
}

char *strchr(const char *str, int character) {
  const auto c = static_cast<char>(character);
  do {
    if (*str == c)
      return const_cast<char *>(str);
  } while (*str++ != '\0');
  return nullptr;
}

int strncmp(const char *str1, const char *str2, size_t n) {
  for (size_t i = 0; i < n; ++i)
    if (str1[i] != str2[i])
      return str1[i] < str2[i] ? -1 : 1;
    else if (str2[i] == '\0' && str1[i] == '\0')
      return 0;
  return 0;
}

int strcmp(const char *str1, const char *str2) {
  return strncmp(str1, str2, SIZE_MAX);
}

char *strncpy(char *LIBC_RESTRICT dst, const char *LIBC_RESTRICT src,
              size_t n) {
  for (size_t i = 0; i < n; ++i) {
    dst[i] = src[i];
    if (src[i] == '\0')
      break;
  }
  return dst;
}

char *strcpy(char *LIBC_RESTRICT dst, const char *LIBC_RESTRICT src) {
  return strncpy(dst, src, SIZE_MAX);
}

char *strcat(char *LIBC_RESTRICT dest, const char *LIBC_RESTRICT src) {
  char *ret = dest;
  for (; *dest != '\0'; ++dest)
    ;
  do {
    *dest++ = *src;
  } while (*src++ != '\0');
  return ret;
}

char *strncat(char *LIBC_RESTRICT dest, const char *LIBC_RESTRICT src,
              size_t n) {
  char *ret = dest;
  for (; *dest != '\0'; ++dest)
    ;
  size_t i = 0;
  for (; i < n; ++i) {
    if (src[i] == '\0')
      break;
    dest[i] = src[i];
  }
  dest[i] = '\0';
  return ret;
}

size_t strlen(const char *src) {
  size_t len = 0;
  for (; src[len] != '\0'; ++len)
    ;
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

char *strstr(const char *str, const char *sub) {
  char *hay = const_cast<char *>(str);
  do {
    char *match_start = hay, *match_end = hay;
    const char *needle = sub;
    do {
      if (*needle == '\0')
        return match_start;
    } while (*match_end++ == *needle++);
  } while (*hay++ != '\0');
  return nullptr;
}

#if TESTING_LIBC
}
}
#endif
