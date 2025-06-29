#ifndef LIBADT_HASH_MAP_H
#define LIBADT_HASH_MAP_H

#include <string.h>
#include <stddef.h>

#include <type_traits>
#include <utility>

namespace adt {

template <typename T, typename EnableIf = void> struct hash_traits {
  // static unsigned long long hash(const T& value);
};

template <typename T>
struct hash_traits<T, std::enable_if_t<std::is_integral_v<T>>> {
  static unsigned long long hash(T value) { return value; };
};

template <typename Key, typename Value, typename Allocator> class hash_map {
  struct entry {
    bool empty : 1;
    bool sentinel : 1;
    std::pair<Key, Value> data;
  };

  size_t bucket(const Key &k) const {
    return hash_traits<Key>::hash(k) & (capacity() - 1);
  }

public:
  class iterator;
  using allocator_type = Allocator;

  hash_map() = default;

  hash_map(const hash_map &other)
      : log2capacity{other.log2capacity}, num_elems{other.num_elems} {
    data = allocator_type::template alloc_array_of<entry>(capacity());
    memcpy(data, other.data, sizeof(entry) * capacity());
  }
  hash_map &operator=(const hash_map &other) {
    log2capacity = other.log2capacity;
    num_elems = other.num_elems;
    allocator_type::free(data);
    data = allocator_type::template alloc_array_of<entry>(capacity());
    memcpy(data, other.data, sizeof(entry) * capacity());
    return *this;
  }

  hash_map(hash_map &&other) { *this = std::move(other); }
  hash_map &operator=(hash_map &&other) {
    std::swap(num_elems, other.num_elems);
    std::swap(log2capacity, other.log2capacity);
    std::swap(data, other.data);
    return *this;
  }

  ~hash_map() { allocator_type::free(data); }

  bool insert(std::pair<Key, Value> kv) {
    reserve(size() + 1);

    size_t idx = bucket(kv.first);
    while (true) {
      if (data[idx].data.first == kv.first)
        return true;
      if (data[idx].empty)
        break;
      if (data[idx].sentinel)
        idx = 0;
      else
        ++idx;
    }

    data[idx].data = std::move(kv);
    data[idx].empty = false;
    num_elems += 1;

    return false;
  }

  bool contains(const Key &k) const { return lookup(k) != nullptr; }

  Value *lookup(const Key &k) const {
    size_t idx = bucket(k);
    while (!data[idx].empty) {
      if (data[idx].data.first == k)
        return &data[idx].data.second;
      if (data[idx].sentinel)
        idx = 0;
    }
    return nullptr;
  }

  iterator begin() { return iterator{data}; }
  iterator end() { return iterator{nullptr}; }

  size_t capacity() const { return 1 << log2capacity; }

  size_t size() const { return num_elems; }
  void reserve(size_t bytes) {
    // Remember to account for sentinel entry (at the end)
    if (bytes + 1 <= capacity() - 1)
      return;

    if (capacity() == 0)
      log2capacity = 6; // 64B cacheline
    else
      log2capacity += 1;

    hash_map NewMap;
    NewMap.log2capacity = log2capacity;
    NewMap.data = allocator_type::template alloc_array_of<entry>(capacity());
    memset(NewMap.data, 0, sizeof(entry) * capacity());
    NewMap.data[(1 << log2capacity) - 1].sentinel = true;
    for (auto &[key, value] : *this)
      NewMap.insert({key, value});

    std::swap(data, NewMap.data);
  }

private:
  size_t log2capacity = 0;
  size_t num_elems = 0;
  entry *data = nullptr;
};

template <typename Key, typename Value, typename Allocator>
class hash_map<Key, Value, Allocator>::iterator {
  friend class hash_map<Key, Value, Allocator>;

  entry *current = nullptr;

  void advance_til_non_empty_or_end() {
    // Search for next non-empty value. Stop at the sentinel.
    while (current->empty && !current->sentinel)
      ++current;

    // If we've hit the sentinel, we're done iterating.
    if (current->sentinel)
      current = nullptr;
  }

  iterator(entry *data) : current{data} {
    if (current) {
      if (current->empty)
        advance_til_non_empty_or_end();
      else if (current->sentinel)
        current = nullptr;
    }
  }

public:
  iterator &operator++() {
    if (current)
      advance_til_non_empty_or_end();
    return *this;
  }

  std::pair<Key, Value> &operator*() { return current->data; }
  const std::pair<Key, Value> &operator*() const { return current->data; }

  friend bool operator==(const iterator &A, const iterator &B) {
    return A.current == B.current;
  }
};

} // namespace adt

#endif
