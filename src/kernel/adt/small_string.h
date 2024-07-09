#ifndef ADT_SMALL_STRING_H
#define ADT_SMALL_STRING_H

#include <stdint.h>
#include <string.h>

#include "adt/optional.h"
#include "allocator.h"

namespace kstd {

template <size_t SmallSize>
class string_storage {
  union storage {
    char small[SmallSize];
    char *big;
  } data;
  uint32_t count = 0;
  bool is_small = true;

protected:
  string_storage(const char *src, size_t n) : count(n), is_small(true) {
    memcpy(data.small, src, n);
  }

  string_storage(const char *src, char *buffer, size_t n)
      : count(n), is_small(false) {
    data.big = buffer;
    memcpy(data.big, src, n);
  }
};

template <typename Alloc, size_t N = sizeof(void *)>
class string_base : string_storage<N> {
public:
  static_assert(N >= sizeof(void *),
                "small size must be at least as big as a pointer");
  using allocator = kstd::allocator<Alloc>;
  static constexpr auto small_string_size = N / sizeof(char);

  string_base(const char *src, size_t n) : string_storage<N>(src, n) {}
  string_base(const char *src, char *buffer, size_t n)
      : string_storage<N>(src, buffer, n) {}

public:
  static optional<string_base> make(char *data, size_t n) {
    if (n <= small_string_size)
      return string_base(data, n);

    char *d = allocator::template alloc_array_of<char>(n);
    if (!d)
      return kstd::none;

    return string_base(data, d, n);
  }
};

class standard_alloc;
extern template class string_base<standard_alloc>;

using string = string_base<standard_alloc>;
template <size_t N> using small_string = string_base<standard_alloc, N>;

} // namespace kstd

#endif
