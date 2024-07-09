#ifndef ADT_RING_BUFFER_H
#define ADT_RING_BUFFER_H

#include <stdint.h>
#include <stddef.h>

namespace kstd {

// base       base + size (data + 1)
// v          v
// XXXXXXXXXXX
//   ^       ^
//   begin   end
template <typename T, size_t N> class ring_buffer {
  static_assert(N >= 2, "two elements of the ring buffer *must* be reserved "
                        "for the begin/end pointers");

  struct cursor {
    T *begin = nullptr;
    T *end = nullptr;
  };

public:
  static constexpr auto capacity = N - 2;

  ring_buffer() : c{data, data + 1} {}

  void clear() { c.begin = data; c.end = data + 1; count = 0; }

  T *forward_wrap(T *p) const {
    return p + 1 == data + capacity ? const_cast<T *>(data) : p + 1;
  }
  T *backward_wrap(T *p) const {
    return p == data ? const_cast<T *>(data) + capacity - 1 : p - 1;
  }

  bool push_back(T val) {
    // Empty case:
    // Before       | After
    // ------------------v------ new val
    //  XXXXXX      |  XXXXXX
    //   ^^         |   ^ ^
    //   se         |   s e
    //
    // Partially-full case (nowrap):
    // Before       | After
    // -------------------v----- new val
    //  XXXXXX      |  XXXXXX
    //   ^ ^        |   ^  ^
    //   s e        |   s  e
    //
    // Partially-full case (wrap):
    // Before       | After (now full)
    // ---------------------v----- new val
    //  XXXXXX      |  XXXXXX
    //   ^   ^      |  ^^
    //   s   e      |  es
    //
    // Full case
    // Before       | After (still full)
    // --------------------------- no val stored
    //  XXXXXX      |  XXXXXX
    //  ^^          |  ^^
    //  es          |  es

    // Next end ptr, accounting for wraparound.
    T *next_end = forward_wrap(c.end);

    // buffer is full
    if (next_end == c.begin)
      return false;

    // push the value
    *c.end = val;

    // advance end
    c.end = next_end;
    ++count;

    return true;
  }

  bool push_front(T val) {
    // Empty case:
    // Before       | After
    // -----------------v------- new val
    //  XXXXXX      |  XXXXXX
    //   ^^         |  ^ ^
    //   se         |  s e
    //
    // Partially-full case (nowrap):
    // Before       | After
    // -----------------v------- new val
    //  XXXXXX      |  XXXXXX
    //   ^ ^        |  ^  ^
    //   s e        |  s  e
    //
    // Partially-full case (wrap):
    // Before       | After (now full)
    // ---------------------v----- new val
    //  XXXXXX      |  XXXXXX
    //   ^   ^      |  ^^
    //   s   e      |  es
    //
    // Full case
    // Before       | After (still full)
    // --------------------------- no val stored
    //  XXXXXX      |  XXXXXX
    //  ^^          |  ^^
    //  es          |  es

    // Next begin ptr, accounting for wraparound.
    T *next_begin = backward_wrap(c.begin);

    // buffer is full
    if (next_begin == c.end)
      return false;

    // push the value
    *c.begin = val;

    // advance end
    c.begin = next_begin;
    ++count;

    return true;
  }

  bool pop_front(T &out) {
    // Empty case:
    // Before       | After
    //  XXXXXX      |  XXXXXX
    //   ^^         |   ^^
    //   se         |   se
    //
    // Partially-full case (nowrap):
    // Before       | After (now empty)
    //  XXXXXX      |  XXXXXX
    //   ^ ^        |    ^^
    //   s e        |    se
    //
    // Partially-full case (wrap):
    // Before       | After (now empty)
    //  XXXXXX      |  XXXXXX
    //   ^   ^      |  ^^
    //   e   s      |  se
    //
    // Full case
    // Before       | After
    //  XXXXXX      |  XXXXXX
    //  ^^          |  ^ ^
    //  es          |  e s

    // Next begin ptr, accounting for wraparound.
    T *next_begin = forward_wrap(c.begin);

    // buffer is empty
    if (next_begin == c.end)
      return false;

    // pop the value
    out = *next_begin;

    // advance begin
    c.begin = next_begin;
    --count;

    return true;
  }

  bool pop_front() {
    T dummy;
    return pop_front(dummy);
  }

  bool pop_back(T &out) {
    // Empty case:
    // Before       | After
    //  XXXXXX      |  XXXXXX
    //   ^^         |   ^^
    //   se         |   se
    //
    // Partially-full case (nowrap):
    // Before       | After (now empty)
    //  XXXXXX      |  XXXXXX
    //   ^ ^        |   ^^
    //   s e        |   se
    //
    // Partially-full case (wrap):
    // Before       | After (now empty)
    //  XXXXXX      |  XXXXXX
    //  ^   ^       |      ^^
    //  e   s       |      se
    //
    // Full case
    // Before       | After
    //  XXXXXX      |  XXXXXX
    //  ^^          |  ^ ^
    //  es          |  e s

    // Next end ptr, accounting for wraparound.
    T *next_end = backward_wrap(c.end);

    // buffer is empty
    if (next_end == c.begin)
      return false;

    // pop the value
    out = *next_end;

    // advance end
    c.end = next_end;
    --count;

    return true;
  }

  bool pop_back() {
    T dummy;
    return pop_back(dummy);
  }

  const T &front() const { return *forward_wrap(c.begin); }
  T &front() { return *forward_wrap(c.begin); }
  const T &back() const { return *backward_wrap(c.end); }
  T &back() { return *backward_wrap(c.end); }

  void write_to_buffer(T *buffer, size_t len) const {
    if (len == 0 || empty())
      return;

    size_t i = 0;
    T *current = c.begin;
    do {
      T *next = forward_wrap(current);
      buffer[i] = *next;
      current = next;
    } while (++i < len && current != c.end);
  }

  class iterator {
    ring_buffer *b = nullptr;
    T *cursor = nullptr;

  public:
    iterator(ring_buffer &b, T *cursor) : b(&b), cursor(cursor) {}

    const T &operator *() const { return *cursor; }
    T &operator *() { return *cursor; }

    iterator &operator++() {
      cursor = b->forward_wrap(cursor);
      return *this;
    }
    friend bool operator==(const iterator &a, const iterator &b) {
      return a.b == b.b && a.cursor == b.cursor;
    }
    friend bool operator!=(const iterator &a, const iterator &b) {
      return !(a == b);
    }
  };

  iterator begin() { return iterator{*this, c.begin + 1}; }
  iterator end() { return iterator{*this, c.end}; }

  auto size() const { return count; }
  auto empty() const { return size() == 0; }

private:
  cursor c{};
  uint64_t count = 0;
  T data[capacity];
};
} // namespace kstd

#endif
