#ifndef UTIL_ALLOCATOR_H
#define UTIL_ALLOCATOR_H

#include "util.h"
#include <stddef.h>

namespace kstd {
template <typename Alloc> struct allocator {
  template <typename T, kstd::Align A = kstd::align_of<T>> static T *alloc() {
    return (T *)Alloc::allocate(sizeof(T), A);
  }
  template <typename T, kstd::Align A = kstd::align_of<T>>
  static T *alloc_array_of(size_t n) {
    return (T *)Alloc::allocate(sizeof(T) * n, A);
  }
  template <typename T, kstd::Align A = kstd::align_of<T>>
  static void free(T *p) {
    Alloc::deallocate(p, A);
  }
};
} // namespace kstd

#endif
