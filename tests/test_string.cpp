#include <gtest/gtest.h>

#include "utils.h"

#include <libc/string.h>

TEST(string_h, strcpy_nonempty) {
  char buffer[] = "foo";
  char dst[sizeof(buffer)];
  char *ret = kstd::strcpy(dst, buffer);
  ASSERT_EQ(ret, dst);
  ASSERT_STREQ(dst, "foo");
}

TEST(string_h, strcpy_empty) {
  char buffer[] = "";
  char dst[sizeof(buffer)];
  char *ret = kstd::strcpy(dst, buffer);
  ASSERT_EQ(ret, dst);
  ASSERT_STREQ(dst, "");
}

TEST(string_h, strcat_empty_to_empty) {
  char dst[10] = "";
  char *ret = kstd::strcat(dst, "");
  ASSERT_EQ(ret, dst);
  ASSERT_STREQ(dst, "");
}

TEST(string_h, strcat_empty_to_nonempty) {
  char dst[10] = "foo";
  char *ret = kstd::strcat(dst, "");
  ASSERT_EQ(ret, dst);
  ASSERT_STREQ(dst, "foo");
}

TEST(string_h, strcat_nonempty_to_nonempty) {
  char dst[10] = "foo ";
  char *ret = kstd::strcat(dst, "bar");
  ASSERT_EQ(ret, dst);
  ASSERT_STREQ(dst, "foo bar");
}

TEST(string_h, strcat_nonempty_to_empty) {
  char dst[10] = "";
  char *ret = kstd::strcat(dst, "foo");
  ASSERT_EQ(ret, dst);
  ASSERT_STREQ(dst, "foo");
}

TEST(string_h, strncat_empty_to_empty) {
  {
    char dst[10] = "";
    char *ret = kstd::strncat(dst, "", 0);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "");
  }
  {
    char dst[10] = "";
    char *ret = kstd::strncat(dst, "", 1);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "");
  }
}

TEST(string_h, strncat_nonempty_to_empty) {
  {
    char dst[10] = "";
    char *ret = kstd::strncat(dst, "foo", 0);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "");
  }
  {
    char dst[10] = "";
    char *ret = kstd::strncat(dst, "foo", 1);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "f");
  }
  {
    char dst[10] = "";
    char *ret = kstd::strncat(dst, "foo", 3);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "foo");
  }
  {
    char dst[10] = "";
    char *ret = kstd::strncat(dst, "foo", 4);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "foo");
  }
}

TEST(string_h, strncat_empty_to_nonempty) {
  {
    char dst[10] = "foo";
    char *ret = kstd::strncat(dst, "", 0);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "foo");
  }
  {
    char dst[10] = "foo";
    char *ret = kstd::strncat(dst, "", 1);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "foo");
  }
  {
    char dst[10] = "foo";
    char *ret = kstd::strncat(dst, "", 3);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "foo");
  }
  {
    char dst[10] = "foo";
    char *ret = kstd::strncat(dst, "", 4);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "foo");
  }
}

TEST(string_h, strncat_nonempty_to_nonempty) {
  {
    char dst[10] = "foo";
    char *ret = kstd::strncat(dst, "bar", 0);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "foo");
  }
  {
    char dst[10] = "foo";
    char *ret = kstd::strncat(dst, "bar", 1);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "foob");
  }
  {
    char dst[10] = "foo";
    char *ret = kstd::strncat(dst, "bar", 3);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "foobar");
  }
  {
    char dst[10] = "foo";
    char *ret = kstd::strncat(dst, "bar", 4);
    ASSERT_EQ(ret, dst);
    ASSERT_STREQ(dst, "foobar");
  }
}

TEST(string_h, memcpy) {
  unsigned buffer1[0x100] = {};
  unsigned buffer2[0x100] = {};

  for (unsigned i = 0; i < 0x100; ++i)
    buffer1[i] = i;

  kstd::memcpy((void *)buffer2, (const void *)buffer1,
               0x100 * sizeof(unsigned));
  for (unsigned i = 0; i < 0x100; ++i)
    ASSERT_EQ(buffer2[i], i);
}

TEST(string_h, memcpy_with_remainder) {
  unsigned buffer1[0x103] = {0};
  unsigned buffer2[0x103] = {0};

  for (unsigned i = 0; i < 0x103; ++i)
    buffer1[i] = i;

  kstd::memcpy((void *)buffer2, (const void *)buffer1,
               0x103 * sizeof(unsigned));
  for (unsigned i = 0; i < 0x103; ++i)
    ASSERT_EQ(buffer2[i], i);
}

TEST(string_h, memset) {
  unsigned char buffer[0x100] = {};
  for (unsigned i = 0; i < 0x100; ++i)
    EXPECT_EQ(buffer[i], 0);

  kstd::memset((void*)buffer, 0xF0, 0x100 * sizeof(unsigned char));
  for (unsigned i = 0; i < 0x100; ++i)
    EXPECT_EQ(buffer[i], 0xF0);

  kstd::memset((void *)buffer, 0x10, 0x10 * sizeof(unsigned char));
  for (unsigned i = 0; i < 0x100; ++i)
    if (i < 0x10)
      EXPECT_EQ(buffer[i], 0x10) << " (i = " << i << ")";
    else
      EXPECT_EQ(buffer[i], 0xF0) << " (i = " << i << ")";
}

TEST(string_h, memset_with_remainder) {
  unsigned char buffer[0x107] = {};
  for (unsigned i = 0; i < 0x107; ++i)
    EXPECT_EQ(buffer[i], 0);

  kstd::memset((void*)buffer, 0xF0, 0x107 * sizeof(unsigned char));
  for (unsigned i = 0; i < 0x107; ++i)
    EXPECT_EQ(buffer[i], 0xF0);

  kstd::memset((void *)buffer, 0x10, 0x10 * sizeof(unsigned char));
  for (unsigned i = 0; i < 0x107; ++i)
    if (i < 0x10)
      EXPECT_EQ(buffer[i], 0x10) << " (i = " << i << ")";
    else
      EXPECT_EQ(buffer[i], 0xF0) << " (i = " << i << ")";
}

TEST(string_h, memmove_backwards_overlap) {
  char buffer[] = "$  test string";
  char *beginning_of_test_string = &buffer[3];
  char *after_dollar_sign = &buffer[1];
  kstd::memmove(after_dollar_sign, beginning_of_test_string,
                ::strlen("test string"));
  ASSERT_STREQ(buffer, "$test stringng");
}

TEST(string_h, memmove_forwards_overlap) {
  char buffer[] = "$test string  ";
  char *beginning_of_test_string = &buffer[1];
  char *two_chars_ahead = beginning_of_test_string + 2;
  kstd::memmove(two_chars_ahead, beginning_of_test_string,
                ::strlen("test string"));
  ASSERT_STREQ(buffer, "$tetest string");
}

TEST(string_h, strchr) {
  char buffer[] = "foo bar";
  char *needle = kstd::strchr(buffer, ' ');
  ASSERT_STREQ(needle, " bar");
  EXPECT_EQ(*needle, ' ');
  EXPECT_EQ(buffer + 3, needle);

  needle = kstd::strchr(buffer, 'x');
  ASSERT_FALSE(needle);

  needle = kstd::strchr(buffer, '\0');
  ASSERT_STREQ(needle, "");
  EXPECT_EQ(*needle, '\0');
  EXPECT_EQ(buffer + 7, needle);
}

TEST(string_h, strlen) {
  EXPECT_EQ(kstd::strlen(""), 0);
  EXPECT_EQ(kstd::strlen("10"), 2);
  EXPECT_EQ(kstd::strlen("10\0foo"), 2);
  EXPECT_EQ(kstd::strlen("foo"), 3);
}

TEST(string_h, strrev) {
  char buffer[] = "foo bar";
  kstd::strrev(buffer, ::strlen(buffer));
  ASSERT_STREQ(buffer, "rab oof");
}

TEST(string_h, strstr_empty_hay_empty_needle) {
  EXPECT_STREQ(kstd::strstr("", ""), "");
}

TEST(string_h, strstr_empty_hay_nonempty_needle) {
  EXPECT_FALSE(kstd::strstr("", "foo"));
}

TEST(string_h, strstr_nonempty_hay_nonempty_needle) {
  EXPECT_STREQ(kstd::strstr("foo bar", "bar"), "bar");
}

TEST(string_h, strstr_nonempty_hay_empty_needle) {
  EXPECT_STREQ(kstd::strstr("foo bar", ""), "foo bar");
}

TEST(string_h, strstr_nonempty_hay_needle_not_found) {
  EXPECT_FALSE(kstd::strstr("foo bar", "baz"));
}

TEST(string_h, memstr_needle_not_found) {
  EXPECT_FALSE(kstd::memstr("", "", 0));
  EXPECT_FALSE(kstd::memstr("foo", "bar", 3));
  EXPECT_FALSE(kstd::memstr("foo foo", "bar", 7));
}

TEST(string_h, memstr_needle_found) {
  EXPECT_TRUE(kstd::memstr("", "", 1));
  EXPECT_TRUE(kstd::memstr("foo", "foo", 4));
  EXPECT_TRUE(kstd::memstr("foo bar", "bar", 7));
}

TEST(string_h, memchr_search_prefix) {
  EXPECT_FALSE((char*)kstd::memchr("abc", (int)'a', 0));
  EXPECT_TRUE((char*)kstd::memchr("abc", (int)'a', 1));
  EXPECT_FALSE((char*)kstd::memchr("abc", (int)'b', 1));
  EXPECT_TRUE((char*)kstd::memchr("abc", (int)'b', 2));
  EXPECT_FALSE((char*)kstd::memchr("abc", (int)'c', 2));
  EXPECT_TRUE((char*)kstd::memchr("abc", (int)'c', 3));
}

TEST(string_h, memchr_needle_found) {
  char *match = (char*)kstd::memchr("abc", (int)'b', 4);
  ASSERT_TRUE(match);
  EXPECT_EQ(*match, 'b');

  match = (char*)kstd::memchr("abc", (int)'\0', 4);
  ASSERT_TRUE(match);
  EXPECT_EQ(*match, '\0');
}

TEST(string_h, memchr_needle_not_found) {
  ASSERT_FALSE((char *)kstd::memchr("abc", (int)'z', 3));
}

TEST(string_h, memcmp_equal) {
  EXPECT_EQ(kstd::memcmp("abc", "abc", 3), 0);
  EXPECT_EQ(kstd::memcmp("abc\0ef", "abc\0ef", 5), 0);
  EXPECT_EQ(kstd::memcmp("", "", 0), 0);
  EXPECT_EQ(kstd::memcmp("abcd", "abce", 3), 0);
  EXPECT_EQ(kstd::memcmp("abfd", "abcg", 2), 0);
  EXPECT_EQ(kstd::memcmp("aefg", "abcd", 1), 0);
  EXPECT_EQ(kstd::memcmp("abcd", "efgh", 0), 0);
}

TEST(string_h, memcmp_lt) {
  EXPECT_EQ(kstd::memcmp("a", "b", 1), -1);
  EXPECT_EQ(kstd::memcmp("aa", "ab", 2), -1);
}

TEST(string_h, memcmp_gt) {
  EXPECT_EQ(kstd::memcmp("b", "a", 1), 1);
  EXPECT_EQ(kstd::memcmp("ab", "aa", 2), 1);
}

TEST(string_h, strcmp) {
  EXPECT_EQ(kstd::strcmp("abc", "abc"), 0);
  EXPECT_EQ(kstd::strcmp("", ""), 0);
}

TEST(string_h, strcmp_lt) {
  EXPECT_EQ(kstd::strcmp("a", "b"), -1);
  EXPECT_EQ(kstd::strcmp("aa", "ab"), -1);
  EXPECT_EQ(kstd::strcmp("a", "ab"), -1);
  EXPECT_EQ(kstd::strcmp("ab", "bb"), -1);
  EXPECT_EQ(kstd::strcmp("abc", "bbc"), -1);
}

TEST(string_h, strcmp_gt) {
  EXPECT_EQ(kstd::strcmp("b", "a"), 1);
  EXPECT_EQ(kstd::strcmp("ab", "aa"), 1);
  EXPECT_EQ(kstd::strcmp("ab", "a"), 1);
  EXPECT_EQ(kstd::strcmp("b", "ab"), 1);
  EXPECT_EQ(kstd::strcmp("bb", "ab"), 1);
  EXPECT_EQ(kstd::strcmp("bbc", "abc"), 1);
}

TEST(string_h, strncmp) {
  EXPECT_EQ(kstd::strncmp("abc", "abc", 0), 0);
  EXPECT_EQ(kstd::strncmp("abc", "abc", 1), 0);
  EXPECT_EQ(kstd::strncmp("abc", "abc", 3), 0);
  EXPECT_EQ(kstd::strncmp("abc", "abc", 10), 0);
  EXPECT_EQ(kstd::strncmp("", "", 0), 0);
  EXPECT_EQ(kstd::strncmp("", "", 3), 0);
  EXPECT_EQ(kstd::strncmp("abcd", "abce", 3), 0);
  EXPECT_EQ(kstd::strncmp("abfd", "abcg", 2), 0);
  EXPECT_EQ(kstd::strncmp("aefg", "abcd", 1), 0);
  EXPECT_EQ(kstd::strncmp("abcd", "efgh", 0), 0);
}

TEST(string_h, strncmp_lt) {
  EXPECT_EQ(kstd::strncmp("a", "b", 2), -1);
  EXPECT_EQ(kstd::strncmp("aa", "ab", 2), -1);
  EXPECT_EQ(kstd::strncmp("a", "ab", 2), -1);
  EXPECT_EQ(kstd::strncmp("abc", "bbc", 1), -1);
  EXPECT_EQ(kstd::strncmp("abc", "bbc", 2), -1);
  EXPECT_EQ(kstd::strncmp("abc", "bbc", 3), -1);
}

TEST(string_h, strncmp_gt) {
  EXPECT_EQ(kstd::strncmp("b", "a", 1), 1);
  EXPECT_EQ(kstd::strncmp("ab", "aa", 2), 1);
  EXPECT_EQ(kstd::strncmp("ab", "a", 2), 1);
  EXPECT_EQ(kstd::strncmp("bbc", "abc", 1), 1);
  EXPECT_EQ(kstd::strncmp("bbc", "abc", 2), 1);
  EXPECT_EQ(kstd::strncmp("bbc", "abc", 3), 1);
}
