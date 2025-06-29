#ifndef LIBADT_SMALL_STRING_H
#define LIBADT_SMALL_STRING_H

#include <stdint.h>
#include <string.h>

#include "optional.h"
#include "util/allocator.h"

namespace adt {

template <size_t SmallSize> class string_storage {
  union storage {
    char small[SmallSize];
    char *big;
  } data;
  uint32_t count = 0;
  bool is_small = true;

public:
  uint32_t size() const { return count; }

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

template <> class string_storage<0> {
  char *data = nullptr;
  uint32_t count = 0;

public:
  uint32_t size() const { return count; }

protected:
  string_storage(const char *src, char *buffer, size_t n) {
    data = buffer;
    memcpy(data, src, n);
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
    if (n < small_string_size)
      return string_base(data, n);

    char *d = allocator::template alloc_array_of<char>(n);
    if (!d)
      return adt::none;

    return string_base(data, d, n);
  }
};

class standard_alloc;

template <size_t N> using small_string = string_base<standard_alloc, N>;
using string = small_string<0>;

} // namespace adt

#endif
