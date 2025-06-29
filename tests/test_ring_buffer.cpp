#include <gtest/gtest.h>

#include "libadt/ring_buffer.h"

TEST(ring_buffer, push_back_adds_elem_to_back) {
  adt::ring_buffer<char, 4> buffer;
  ASSERT_EQ(buffer.size(), 0);
  ASSERT_TRUE(buffer.push_back('a'));
  EXPECT_EQ(buffer.back(), 'a');
  EXPECT_EQ(buffer.size(), 1);
  ASSERT_TRUE(buffer.push_back('b'));
  EXPECT_EQ(buffer.back(), 'b');
  EXPECT_EQ(buffer.size(), 2);
}

TEST(ring_buffer, pop_back_takes_elem_from_back) {
  adt::ring_buffer<char, 4> buffer{{'a', 'b', 'c', 'd'}};
  auto back = buffer.pop_back();
  ASSERT_TRUE(back.has_value());
  EXPECT_EQ(*back, 'd');
  back = buffer.pop_back();
  ASSERT_TRUE(back.has_value());
  EXPECT_EQ(*back, 'c');
  back = buffer.pop_back();
  ASSERT_TRUE(back.has_value());
  EXPECT_EQ(*back, 'b');
  back = buffer.pop_back();
  ASSERT_TRUE(back.has_value());
  EXPECT_EQ(*back, 'a');
  back = buffer.pop_back();
  ASSERT_EQ(back, adt::none);
}

TEST(ring_buffer, push_back_saturates) {
  adt::ring_buffer<char, 2> buffer;
  ASSERT_TRUE(buffer.push_back('a'));
  ASSERT_TRUE(buffer.push_back('a'));
  EXPECT_EQ(buffer.size(), 2);
  ASSERT_FALSE(buffer.push_back('a'));
  EXPECT_EQ(buffer.size(), 2);
}

TEST(ring_buffer, push_pop) {
  adt::ring_buffer<int, 4> buffer;

  for (int i = 0; i < 4; ++i)
    ASSERT_TRUE(buffer.push_back(i));
  EXPECT_EQ(buffer.size(), 4);

  for (int i = 3; i >= 0; --i)
    EXPECT_EQ(buffer.pop_back(), i);
  EXPECT_EQ(buffer.size(), 0);

  for (int i = 0; i < 4; ++i)
    ASSERT_TRUE(buffer.push_front(i));
  EXPECT_EQ(buffer.size(), 4);

  for (int i = 3; i >= 0; --i)
    EXPECT_EQ(buffer.pop_front(), i);
  EXPECT_EQ(buffer.size(), 0);
}

TEST(ring_buffer, push_front_adds_elem_to_front) {
  adt::ring_buffer<char, 4> buffer;
  ASSERT_EQ(buffer.size(), 0);
  ASSERT_TRUE(buffer.push_front('a'));
  EXPECT_EQ(buffer.front(), 'a');
  EXPECT_EQ(buffer.size(), 1);
  ASSERT_TRUE(buffer.push_front('b'));
  EXPECT_EQ(buffer.front(), 'b');
  EXPECT_EQ(buffer.size(), 2);
}

TEST(ring_buffer, push_front_saturates) {
  adt::ring_buffer<char, 2> buffer;
  ASSERT_TRUE(buffer.push_front('a'));
  ASSERT_TRUE(buffer.push_front('a'));
  ASSERT_EQ(buffer.size(), 2);
  ASSERT_FALSE(buffer.push_front('a'));
  ASSERT_EQ(buffer.size(), 2);
}

TEST(ring_buffer, clear_ring_buffer) {
  adt::ring_buffer<char, 1> buffer;
  buffer.clear();
  EXPECT_EQ(buffer.size(), 0);
  buffer.push_back('a');
  buffer.clear();
  EXPECT_EQ(buffer.size(), 0);
}

TEST(ring_buffer, check_empty_or_full) {
  {
    adt::ring_buffer<int, 2> b{{1, 2}};
    EXPECT_FALSE(b.empty());
    EXPECT_TRUE(b.full());
    b.pop_back();
    b.pop_back();
    EXPECT_TRUE(b.empty());
    EXPECT_FALSE(b.full());
  }
  {
    adt::ring_buffer<int, 1> b{{1}};
    EXPECT_FALSE(b.empty());
    EXPECT_TRUE(b.full());
    b.pop_back();
    EXPECT_TRUE(b.empty());
    EXPECT_FALSE(b.full());
  }
  {
    adt::ring_buffer<int, 2> b{{1}};
    EXPECT_FALSE(b.empty());
    EXPECT_FALSE(b.full());
    b.pop_back();
    EXPECT_TRUE(b.empty());
    EXPECT_FALSE(b.full());
  }
  {
    adt::ring_buffer<int, 2> b;
    EXPECT_TRUE(b.empty());
    EXPECT_FALSE(b.full());
    b.push_back(1);
    EXPECT_FALSE(b.empty());
    EXPECT_FALSE(b.full());
    b.push_back(1);
    EXPECT_FALSE(b.empty());
    EXPECT_TRUE(b.full());
  }
}
