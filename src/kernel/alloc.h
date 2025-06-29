#ifndef KERNEL_ALLOC_H
#define KERNEL_ALLOC_H

#include <assert.h>
#include <string.h>
#include <stddef.h>

#include "util.h"

namespace alloc {

enum protection {
  READ_ONLY,
  READ_WRITE,
  EXEC,
};

void init();

void *alloc(size_t count, kstd::Align alignment, protection p);
void free(void *data);

inline void *alloc_zeroed(size_t count, kstd::Align alignment, protection p) {
  assert(p != READ_ONLY && "can't zero-out read-only memory (add support?)");
  void *ret = alloc(count, alignment, p);
  memset(ret, 0, count);
  return ret;
}

template <typename T, kstd::Align A = kstd::align_of<T>>
T *one(protection p = protection::READ_WRITE) {
  return (T *)alloc(sizeof(T), A, p);
}
template <typename T, kstd::Align A = kstd::align_of<T>>
T *array_of(size_t n, protection p = protection::READ_WRITE) {
  return (T *)alloc(sizeof(T) * n, A, p);
}
template <typename T, kstd::Align A = kstd::align_of<T>>
T *zeroed_array_of(size_t n, protection p = protection::READ_WRITE) {
  return (T *)alloc_zeroed(sizeof(T) * n, A, p);
}
} // namespace alloc

namespace kstd {
struct standard_alloc {
  static void *allocate(size_t n, kstd::Align alignment) {
    return alloc::alloc(n, alignment, alloc::protection::READ_WRITE);
  }
  static void deallocate(void *p, kstd::Align alignment) { alloc::free(p); }
};
} // namespace kstd

#endif
