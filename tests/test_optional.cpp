#include <sstream>

#include <gtest/gtest.h>

#include "libadt/optional.h"

std::ostream& operator<<(std::ostream &os, adt::none_t) {
  return os << "none";
}

TEST(optional, empty) {
  adt::optional<char> foo;
  EXPECT_FALSE(foo);
  EXPECT_FALSE(foo.has_value());
  EXPECT_EQ(foo, adt::none);
}

TEST(optional, some) {
  adt::optional<char> foo = 'a';
  EXPECT_TRUE(foo);
  EXPECT_TRUE(foo.has_value());
  EXPECT_EQ(*foo, 'a');
}

TEST(optional, none) {
  adt::optional<int> foo = adt::none;
  EXPECT_FALSE(foo);
  EXPECT_FALSE(foo.has_value());
  EXPECT_EQ(foo, adt::none);
}

class DtorTracker {
  bool *Ran = nullptr;

public:
  explicit DtorTracker(bool &Ran) : Ran(&Ran) { *this->Ran = false; }
  DtorTracker(DtorTracker &&Other) : Ran(Other.Ran) {
    Other.Ran = nullptr;
  }
  ~DtorTracker() noexcept {
    if (Ran)
      *Ran = true;
  }
};

TEST(optional, dtor_runs_dtor) {
  bool DtorRan = false;
  { adt::optional<DtorTracker> Test{DtorTracker{DtorRan}}; }
  ASSERT_TRUE(DtorRan);
}

TEST(optional, assign_none_runs_dtor) {
  bool DtorRan = false;
  adt::optional<DtorTracker> Test{DtorTracker{DtorRan}};
  ASSERT_FALSE(DtorRan);
  Test = adt::none;
  ASSERT_TRUE(DtorRan);
}
