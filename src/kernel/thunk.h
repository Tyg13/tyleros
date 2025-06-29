#ifndef THUNK_H
#define THUNK_H

#include <algorithm>
#include <optional>
#include <tuple>

#include "alloc.h"

template <typename T> class Thunk {
public:
  template <typename... ArgsT>
  explicit Thunk(T (*F)(ArgsT...), ArgsT... Args)
      : F{reinterpret_cast<void *>(F)}, Getter{&Thunk<T>::compute<ArgsT...>},
        Deleter(&Thunk::delete_fn<ArgsT...>),
        ParamData{(char *)alloc::one<std::tuple<ArgsT...>>()} {
    new (ParamData) std::tuple<ArgsT...>{Args...};
  }

  Thunk(const Thunk<T> &) = delete;
  Thunk &operator=(const Thunk<T> &) = delete;

  Thunk(Thunk<T> &&Other)
      : Cached{std::move(Other.Cached)}, F{Other.F}, Getter{Other.Getter},
        Deleter{Other.Deleter}, ParamData{Other.ParamData} {
    Other.clear();
  }

  Thunk &operator=(Thunk<T> &&Other) {
    if (&Other != *this) {
      F = Other.F;
      Getter = Other.Getter;
      Deleter = Other.Getter;
      ParamData = Other.Getter;
      Cached = Other.Cached;
      Other.clear();
    }
    return *this;
  }

  ~Thunk() { (this->*Deleter)(ParamData); }

  T operator*() {
    if (!Cached) {
      Cached = (this->*Getter)();
    }
    return *Cached;
  }

  template <typename... ArgsT> T compute() {
    return std::apply(reinterpret_cast<T (*)(ArgsT...)>(this->F),
                      data<ArgsT...>());
  }

private:
  template <typename... ArgsT> void delete_fn(char *Data) {
    alloc::free(&data<ArgsT...>());
  }

  void do_nothing(char *) {}
  void clear() {
    F = nullptr;
    Getter = nullptr;
    Deleter = &Thunk<T>::do_nothing;
    ParamData = nullptr;
  }

  template <typename... ArgsT> std::tuple<ArgsT...> &data() {
    return *reinterpret_cast<std::tuple<ArgsT...> *>(ParamData);
  }

  template <typename... ArgsT> const std::tuple<ArgsT...> &data() const {
    return const_cast<Thunk<T> *>(this)->data<ArgsT...>();
  }

  template <unsigned Idx, typename... ArgsT> auto get() const {
    return data<ArgsT...>().template get<Idx>();
  }

  std::optional<T> Cached;
  T (Thunk::*Getter)() = nullptr;
  void (Thunk::*Deleter)(char *) = nullptr;
  void *F = nullptr;
  char *ParamData = nullptr;
};

template <typename RetT, typename ...ArgsT>
Thunk<RetT> make_thunk(RetT (*F)(ArgsT...), ArgsT... Args) {
  return Thunk<RetT>{F, Args...};
}

#endif
