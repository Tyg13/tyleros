#ifndef LIBADT_UNIQUE_PTR_H
#define LIBADT_UNIQUE_PTR_H

#include <cstddef>
#include <utility>

namespace adt {

template <typename T, typename Deleter>
class unique_ptr {
public:
  explicit unique_ptr() = default;
  explicit unique_ptr(T *data) : storage{data} {}
  unique_ptr(std::nullptr_t) : storage{nullptr} {}

  unique_ptr(const unique_ptr &) = delete;
  unique_ptr &operator=(const unique_ptr &) = delete;

  unique_ptr(unique_ptr &&other) { std::swap(storage, other.storage); }
  unique_ptr &operator=(unique_ptr &&other) {
    std::swap(storage, other.storage);
  }

  ~unique_ptr() { reset(); }
  void reset() {
    deleter(storage);
    storage = nullptr;
  }
  T *release() {
    T *ret = storage;
    storage = nullptr;
    return ret;
  }

  explicit operator bool() const { return storage != nullptr; }
  const T *get() const { return storage; }
  T *get() { return storage; }

  const T &operator*() const { return *storage; }
  T &operator*() { return *storage; }

  const T* operator->() const { return storage; }
  T* operator->() { return storage; }

private:
  [[no_unique_address]] Deleter deleter;
  T *storage = nullptr;
};

} // namespace adt

#endif
