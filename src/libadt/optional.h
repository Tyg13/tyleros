#ifndef LIBADT_OPTIONAL_H
#define LIBADT_OPTIONAL_H

#include <utility>

namespace adt {

struct none_t {};
static inline none_t none;

template <typename T> class optional_storage {
  union storage {
    T value;
    storage() {}
    ~storage() {}
    storage(const T &value) : value(value) {}
    storage(T &&value) : value(std::move(value)) {}
  } data;
  bool has_value;

public:
  optional_storage() : has_value{false} {}
  optional_storage(const T &value) : data{value}, has_value{true} {}
  optional_storage(T &&value) : data{std::move(value)}, has_value{true} {}
  ~optional_storage() { destroy(); }

  void destroy() {
    if (has_value)
      data.value.~T();
    has_value = false;
  }

  bool has_data() const { return has_value; }
  void set_value(const T &value) {
    data.value = value;
    has_value = true;
  }
  void set_value(T &&value) {
    data.value = std::move(value);
    has_value = true;
  }

  const T &get() const & { return data.value; }
  T &get() & { return data.value; }
  T &&get() && {
    has_value = false;
    return std::move(data.value);
  }
};

template <typename T> struct optional {
  optional_storage<T> storage;

public:
  optional() : storage{} {}
  optional(const T &value) : storage(value) {}
  optional(T &&value) : storage(std::move(value)) {}
  optional(none_t) : optional() {}
  ~optional() = default;
  optional(const optional &other) : optional() { *this = other; }
  optional(optional &&other) : optional() { *this = std::move(other); }
  optional &operator=(const optional &other) {
    if (this == &other)
      return *this;

    storage.destroy();
    if (other.storage.has_data())
      storage.set_value(other.storage.get());
    return *this;
  }
  optional &operator=(optional &&other) {
    storage.destroy();
    if (other.storage.has_data()) {
      storage.set_value(std::move(other).storage.get());
    }
    return *this;
  }

  const T &operator*() const { return storage.get(); }
  T &operator*() { return storage.get(); }
  const T *operator->() const { return &storage.get(); }
  T *operator->() { return &storage.get(); }

  bool has_value() const { return storage.has_data(); }
  explicit operator bool() const { return has_value(); }

  optional &operator=(T value) {
    *this = optional(value);
    return *this;
  }

  optional &operator=(none_t) {
    storage.destroy();
    return *this;
  }

  friend bool operator==(const optional &lhs, const optional &rhs) {
    if (lhs && rhs)
      return *lhs == *rhs;
    return lhs && rhs;
  }

  friend bool operator!=(const optional &lhs, const optional &rhs) {
    return !(lhs == rhs);
  }

  friend bool operator==(const optional &lhs, const T &rhs) {
    return lhs && *lhs == rhs;
  }
  friend bool operator!=(const optional &lhs, const T &rhs) {
    return !(lhs == rhs);
  }

  friend bool operator==(const T &lhs, const optional &rhs) {
    return rhs && *rhs == lhs;
  }
  friend bool operator!=(const T &lhs, const optional &rhs) {
    return !(lhs == rhs);
  }

  friend bool operator==(const optional &lhs, const none_t &) { return !lhs; }
  friend bool operator!=(const optional &lhs, const none_t &) { return lhs; }
};

} // namespace adt

#endif
