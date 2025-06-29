#ifndef LIBADT_BUFFER_H
#define LIBADT_BUFFER_H

#include <stddef.h>

namespace adt {

template <typename T> struct buffer {
  buffer() = default;
  buffer(T *Data, size_t Size) : Data{Data}, Size{Size} {}

  size_t size() const { return Size; }

  T* data() { return Data; }
  const T* data() const { return Data; }

  explicit operator bool() const { return Data != nullptr; }

  T &operator[](size_t N) { return Data[N]; }
  const T &operator[](size_t N) const { return Data[N]; }

private:
  T *Data = nullptr;
  size_t Size = 0;
};
} // namespace adt

#endif
