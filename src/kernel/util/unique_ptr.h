#ifndef KERNEL_UTIL_UNIQUE_PTR_H
#define KERNEL_UTIL_UNIQUE_PTR_H

#include <cstddef>

#include "alloc.h"

#include "libadt/unique_ptr.h"

namespace kstd {

template <typename T> struct standard_deleter {
  void operator()(void *data) {
    kstd::standard_alloc::deallocate(data, kstd::align_of<T>);
  }
};

template <typename T, typename Allocator = kstd::standard_deleter<T>>
struct unique_ptr : adt::unique_ptr<T, Allocator> {
  using adt::unique_ptr<T, Allocator>::unique_ptr;
};

static_assert(sizeof(unique_ptr<char>) == sizeof(char *));
} // namespace kstd

#endif
