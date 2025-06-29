#ifndef LIBADT_RING_BUFFER_H
#define LIBADT_RING_BUFFER_H

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "./optional.h"
#include "./array.h"

namespace adt {

// Statically sized generic ring buffer which stores `N` elems of type `T`.
template <typename T, size_t N> class ring_buffer {
public:
  using elem_type = T;
  static constexpr auto capacity = N;

  ring_buffer() = default;
  template <size_t M> ring_buffer(const elem_type (&input)[M]) {
    static_assert(M <= N, "input array too big!");
    memcpy(storage, input, M * sizeof(elem_type));
    _begin = 0;
    _end = M == N ? 0 : M;
    count = M;
  }
  ring_buffer(const ring_buffer &) = default;
  ring_buffer &operator=(const ring_buffer &) = default;
  ~ring_buffer() { clear(); }

  void clear() {
    if constexpr (!std::is_trivially_destructible_v<elem_type>)
      for (elem_type &e : *this)
        e.~elem_type();
    _begin = _end = 0;
    count = 0;
  }

  static size_t forward_wrap(size_t i) { return i + 1 < capacity ? i + 1 : 0; }
  static size_t backward_wrap(size_t i) { return i > 0 ? i - 1 : capacity - 1; }

  bool push_back(elem_type val) {
    // Empty case:
    // Before       | After
    //  ......      |  .X....
    //   ^          |   ^^
    //   s          |   se
    //   e
    //
    // Partially-full case (nowrap):
    // Before       | After
    //  .XX...      |  .XXX..
    //   ^ ^        |   ^  ^
    //   s e        |   s  e
    //
    // Partially-full case (wrap):
    // Before       | After (now full)
    //  XXXXX.      |  XXXXXX
    //  ^    ^      |  ^
    //  s    e      |  s
    //              |  e
    //
    // Full case
    // Before       | After (still full)
    //  XXXXXX      |  XXXXXX
    //   ^          |   ^
    //   s          |   s
    //   e          |   e

    if (full())
      return false;

    // Push the value and advance end ptr, accounting for wraparound.
    storage[_end] = val;
    _end = forward_wrap(_end);
    ++count;

    return true;
  }

  bool push_front(elem_type val) {
    // Empty case:
    // Before       | After
    //  ......      |  X.....
    //   ^          |  ^^
    //   s          |  se
    //   e
    //
    // Partially-full case (nowrap):
    // Before       | After
    //  .XX...      |  XXX...
    //   ^ ^        |  ^  ^
    //   s e        |  s  e
    //
    // Partially-full case (wrap):
    // Before       | After (now full)
    //  .XXXXX      |  XXXXXX
    //  ^^          |  ^
    //  es          |  s
    //              |  e
    //
    // Full case
    // Before       | After (still full)
    //  XXXXXX      |  XXXXXX
    //  ^           |  ^
    //  s           |  s
    //  e           |  e
    //
    if (full())
      return false;

    // Push the value and decrement begin ptr, accounting for wraparound.
    _begin = backward_wrap(_begin);
    storage[_begin] = val;
    ++count;

    return true;
  }

  adt::optional<elem_type> pop_front() {
    // Empty case:
    // Before       | After
    //  ......      |  ......
    //   ^          |   ^
    //   b          |   b
    //   e          |   e
    //
    // Partially-full case (nowrap):
    // Before       | After
    //  .XX...      |  ..X...
    //   ^ ^        |    ^^
    //   b e        |    be
    //
    // Partially-full case (wrap):
    // Before       | After
    //  .....X      |  ......
    //  ^    ^      |  ^
    //  e    b      |  b
    //              |  e
    //
    // Full case
    // Before       | After
    //  XXXXXX      |  .XXXXX
    //  ^           |  ^^
    //  b           |  eb
    //  e           |
    //
    if (empty())
      return adt::none;

    // Pop the element and increment begin ptr.
    elem_type ret = storage[_begin];
    _begin = forward_wrap(_begin);
    --count;

    return ret;
  }

  adt::optional<elem_type> pop_back() {
    // Empty case:
    // Before       | After
    //  ......      |  ......
    //   ^          |   ^
    //   s          |   s
    //   e          |   e
    //
    // Partially-full case (nowrap):
    // Before       | After
    //  .XX...      |  .X....
    //   ^ ^        |   ^^
    //   s e        |   se
    //
    // Partially-full case (wrap):
    // Before       | After
    //  ....XX      |  ....X.
    //  ^   ^       |      ^^
    //  e   s       |      se
    //
    // Full case
    // Before       | After
    //  XXXXXX      |  XXXXX.
    //  ^           |  ^    ^
    //  s           |  s    e
    //  e           |
    if (empty())
      return adt::none;

    _end = backward_wrap(_end);
    elem_type ret = storage[_end];
    --count;

    return ret;
  }

  const elem_type &front() const { return storage[_begin]; }
  elem_type &front() { return storage[_begin]; }
  const elem_type &back() const { return storage[backward_wrap(_end)]; }
  elem_type &back() { return storage[backward_wrap(_end)]; }

private:
  template <bool is_const> class _iterator {
    using elem_ty = std::conditional_t<is_const, const elem_type, elem_type>;
    using buffer_ty =
        std::conditional_t<is_const, const ring_buffer, ring_buffer>;
    elem_ty *data = nullptr;
    size_t idx = 0;

  public:
    _iterator(buffer_ty &b, size_t idx) : data(b.storage.data()), idx{idx} {}

    const elem_ty &operator*() const { return data[idx]; }
    elem_ty &operator*() { return data[idx]; }

    _iterator &operator++() {
      idx = ring_buffer::forward_wrap(idx);
      return *this;
    }

    friend bool operator==(const _iterator &a, const _iterator &b) {
      return a.data == b.data && a.idx == b.idx;
    }

    friend bool operator!=(const _iterator &a, const _iterator &b) {
      return !(a == b);
    }
  };

public:
  using iterator = _iterator</*is_const=*/false>;
  using const_iterator = _iterator</*is_const=*/true>;

  iterator begin() { return iterator{*this, _begin}; }
  iterator end() { return iterator{*this, _end}; }
  const_iterator begin() const { return iterator{*this, _begin}; }
  const_iterator end() const { return iterator{*this, _end}; }

  size_t size() const { return count; }
  bool empty() const { return size() == 0; }
  bool full() const { return size() == capacity; }

private:
  adt::array<elem_type, capacity> storage;
  size_t _begin = 0;
  size_t _end = 0;
  size_t count = 0;
};

} // namespace adt

#endif
