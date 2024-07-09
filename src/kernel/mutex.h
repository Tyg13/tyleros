#ifndef MUTEX_H
#define MUTEX_H

#include "scheduler.h"
#include "adt/optional.h"
#include "timing.h"

#include <utility>

namespace kstd {

struct mutex {
  mutex() : locked{false} {}
  bool try_acquire() {
    return __sync_bool_compare_and_swap(&locked, false, true);
  }
  void acquire() {
    while (!try_acquire())
      scheduler::yield();
  }
  void release() { locked = false; }

  struct already_locked {};
  struct guard {
    guard() = default;
    guard(mutex &mtx) : mtx(&mtx) { mtx.acquire(); }
    guard(mutex &mtx, mutex::already_locked) : mtx(&mtx) {}
    ~guard() { release(); }

    void release() {
      if (mtx)
        mtx->release();
      mtx = nullptr;
    }

    guard(const guard &) = delete;
    guard& operator=(const guard &) = delete;

    guard(guard &&other) { *this = std::move(other); }
    guard &operator=(guard &&other) {
      std::swap(mtx, other.mtx);
      return *this;
    }

  private:
    mutex *mtx = nullptr;
  };
  guard lock() { return guard{*this}; }

private:
  bool locked = false;
};

template <typename T> class mutex_handle {
  mutex::guard lock{};
  T *data = nullptr;

  friend class optional_storage<mutex_handle<T>>;
  mutex_handle() = default;

public:
  mutex_handle(mutex &m, T &data) : lock{m}, data{&data} {}
  mutex_handle(mutex &m, T &data, mutex::already_locked l)
      : lock{m, l}, data{&data} {}

  mutex_handle(const mutex_handle &) = delete;
  mutex_handle &operator=(const mutex_handle &) = delete;

  mutex_handle(mutex_handle &&other)
      : lock{std::move(other.lock)}, data{other.data} {
    other.data = nullptr;
  }
  mutex_handle &operator=(mutex_handle &&other) {
    lock = std::move(other.lock);
    data = other.data;
    other.data = nullptr;
    return *this;
  }

  T &operator*() { return *data; }
  const T &operator*() const { return *data; }
  T *operator->() { return data; }
  const T *operator->() const { return data; }

  void release() {
    lock.release();
    data = nullptr;
  }
};

template <typename T> struct managed_by_mutex {
  managed_by_mutex() = default;
  explicit managed_by_mutex(T data) : m{}, data{data} {}

public:
  mutex_handle<T> lock() { return mutex_handle{m, data}; }

  kstd::optional<mutex_handle<T>> try_lock() {
    if (m.try_acquire())
      return mutex_handle{m, data, mutex::already_locked{}};
    return kstd::none;
  }

  kstd::optional<mutex_handle<T>> try_lock_for(microseconds us) {
    uint64_t waited = 0;
    while (waited < us.val) {
      if (auto lock = try_lock())
        return lock;
      busy_sleep(1_us);
      ++waited;
    }
    return kstd::none;
  }

private:
  mutex m;
  T data;
};

template <typename T>
class optional_storage<mutex_handle<T>> {
  using handle = mutex_handle<T>;
  handle h;

public:
  optional_storage() = default;
  optional_storage(const handle &h) = delete;
  optional_storage(handle &&h) : h{std::move(h)} {}
  ~optional_storage() = default;

  void destroy() { h.release(); }
  bool has_data() const { return h.data != nullptr; }
  void set_value(handle &&h) { this->h = std::move(h); }

  const handle &get() const & { return h; }
  handle &get() & { return h; }
  handle &&get() && { return h; }
};
static_assert(sizeof(optional<mutex_handle<int>>) == sizeof(mutex_handle<int>));

} // namespace kstd

#endif
