#ifndef KERNEL_ATOMIC_H
#define KERNEL_ATOMIC_H

namespace kstd {
  template <typename T>
  class atomic {
    T val{};
  public:
    atomic() = default;
    atomic(T v) { set(v); }

    void set(T v) {
      __atomic_store_n(&val, v, __ATOMIC_RELEASE);
    }
    T get() const {
      return __atomic_load_n(&val, __ATOMIC_ACQUIRE);
    }
  };
}

#endif
