#ifndef LIBADT_ARRAY_H
#define LIBADT_ARRAY_H

#include <stddef.h>

#include <utility>

namespace adt {

template <typename T, size_t N> struct array {
  constexpr array(T elem) {
    for (size_t i = 0; i < N; ++i)
      _data[i] = elem;
  }
  constexpr array() : array(T{}) {}
  constexpr T *begin() { return _data; }
  constexpr const T *begin() const { return _data; }
  constexpr T *end() { return _data + N; }
  constexpr const T *end() const { return _data + N; }
  constexpr T &operator[](size_t idx) & { return _data[idx]; }
  constexpr const T &operator[](size_t idx) const & { return _data[idx]; }
  constexpr T &&operator[](size_t idx) && { return std::move(_data[idx]); }
  constexpr void clear() { *this = array(); }

  constexpr T *data() { return _data; }
  constexpr const T *data() const { return _data; }

  constexpr size_t size() const { return N; }

  constexpr operator T*() { return data(); }
  constexpr operator const T*() const { return data(); }

private:
  T _data[N];
};

} // namespace adt

#endif
