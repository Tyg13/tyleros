#ifndef LIBADT_RANGE_H
#define LIBADT_RANGE_H

#include <stddef.h>
#include <iterator>

namespace adt {

template <typename T> class range {
public:
  range() = default;
  range(T *begin, T *end) : _begin{begin}, _end{end} {}

  auto begin() const { return _begin; }
  auto begin() { return _begin; }
  auto end() const { return _end; }
  auto end() { return _end; }
  const T &operator[](size_t idx) const { return _begin[idx]; }
  T &operator[](size_t idx) { return _begin[idx]; }

private:
  T *_begin = nullptr;
  T *_end = nullptr;
};

template <typename Container> auto make_range(Container &&C) {
  using std::begin, std::end;
  return range(begin(C), end(C));
}

} // namespace adt

#endif
