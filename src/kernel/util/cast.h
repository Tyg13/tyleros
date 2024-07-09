#ifndef UTIL_CAST_H
#define UTIL_CAST_H

#include <utility>
#include <assert.h>

struct TestBase {
  enum Tag { A, B } tag;
};

struct DerivedA : public TestBase {
  static bool classof(const TestBase *v) { return v->tag == A; }
  int val = 1;
};
struct DerivedB : public TestBase {
  static bool classof(const TestBase *v) { return v->tag == B; }
  int val = 2;
};

namespace kstd {
namespace detail {
template <typename To, typename From> struct CastTraits {
  using ToPtr = To *;
};
template <typename To, typename From> struct CastTraits<To, const From> {
  using ToPtr = const To *;
};
} // namespace detail

template <typename To, typename From> bool isa(From *v) {
  return To::classof(v);
}

template <typename To, typename From> auto try_cast(From *v) {
  using ToPtr = detail::CastTraits<To, From>::ToPtr;
  return isa<To>(v) ? reinterpret_cast<ToPtr>(v) : nullptr;
}

template <typename To, typename From> auto cast(From *v) {
  using ToPtr = detail::CastTraits<To, From>::ToPtr;
  assert(isa<To>(v) && "not a valid cast!");
  return isa<To>(v) ? reinterpret_cast<ToPtr>(v) : nullptr;
}
} // namespace kstd

inline int foo() {
  DerivedA A;
  DerivedB B;
  const TestBase *ptr = &A;
  if (const DerivedA *AA = kstd::try_cast<DerivedA>(ptr)) {
    return AA->val;
  }
  TestBase &ref = B;
  if (const DerivedB *BB = kstd::try_cast<DerivedB>(&ref)) {
    return BB->val;
  }
  return true;
}

#endif
