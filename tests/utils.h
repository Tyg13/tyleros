#ifndef TESTS_UTILS_H
#define TESTS_UTILS_H

#include <stddef.h>

namespace test {
constexpr inline size_t strlen(const char *s) {
  unsigned i = 0;
  for (; s[i] != '\0'; ++i);
  return i;
}
}

#endif
