#ifndef UTIL_ALLOCATOR_H
#define UTIL_ALLOCATOR_H

#include <stddef.h>

#include "new.h"

namespace kstd {
template <typename T> struct allocator {
  T *allocate(size_t n) { return reinterpret_cast<T *>(::operator new(n)); }
  void deallocate(T *p) { ::operator delete(p); }
  template <typename U> using rebind = allocator<U>;
};
} // namespace kstd

#endif
