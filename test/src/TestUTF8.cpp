// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/UTF8.hpp"
#include "util/StringUtil.hpp"
#include "util/Macros.hpp"
#include "TestUtil.hpp"

#include <cassert>
#include <string.h>

static const char *const valid[] = {
  "",
  " ",
  "foo",
  "\x01",
  "\x7f",
  "\xc3\xbc",
  "\xd7\x9e",
  "\xe7\x9b\xae",
};

static const char *const invalid[] = {
  "\x80",
  "\xff",
  "\xc3",
  "\xd7",
  "\xe7",
  "\xe7\x9b",
};

static const struct {
  const char *value;
  size_t length;
} length[] = {
  { "", 0 },
  { " ", 1 },
  { "foo", 3 },
  { "\xc3\xbc", 1 },
  { "\xd7\x9e", 1 },
  { "\xe7\x9b\xae", 1 },
};

static const struct {
  char ch;
  const char *utf8;
} latin1_chars[] = {
  { 0, "" },
  { ' ', " " },
  { '\xfc', "\xc3\xbc", },
};

static const struct {
  const char *input, *output;
} crop[] = {
  { "", "" },
  { " ", " " },
  { "foo\xc3\xbc", "foo\xc3\xbc", },
  { "foo\xc3", "foo", },
  { "foo\xe7\x9b\xae", "foo\xe7\x9b\xae", },
  { "foo\xe7\x9b", "foo", },
  { "foo\xe7", "foo", },
};

static size_t
MyLengthUTF8(const char *p)
{
  for (size_t length = 0;; ++length) {
    if (*p == 0)
      return length;

    size_t s = SequenceLengthUTF8(p);
    assert(s > 0);
    p += s;
  }
}

static bool
MyValidateUTF8(const char *p)
{
  while (true) {
    if (*p == 0)
      return true;

    size_t s = SequenceLengthUTF8(p);
    if (s == 0)
      return false;

    p += s;
  }
}

static constexpr struct {
  const char *src;
  size_t truncate, dest_size;
  const char *expected_result;
} truncate_string_tests[] = {
  { "", 100, 100, "" },
  { "a", 100, 100, "a" },
  { "abc", 100, 100, "abc" },
  { "abc", 100, 4, "abc" },
  { "abc", 100, 3, "ab" },
  { "abc", 100, 2, "a" },
  { "abc", 100, 1, "" },
  { "foo\xc3\xbc", 100, 100, "foo\xc3\xbc", },
  { "foo\xc3\xbc", 4, 100, "foo\xc3\xbc", },
  { "foo\xc3\xbc", 3, 100, "foo", },
  { "foo\xc3\xbc", 100, 6, "foo\xc3\xbc", },
  { "foo\xc3\xbc", 100, 5, "foo", },
  { "foo\xc3\xbc", 100, 4, "foo", },
  { "foo\xc3\xbc", 100, 3, "fo", },
  { "foo\xe7\x9b\xae", 4, 7, "foo\xe7\x9b\xae", },
  { "foo\xe7\x9b\xae", 3, 7, "foo", },
  { "foo\xe7\x9b\xae", 4, 6, "foo", },
};

static void
TestTruncateString()
{
  for (const auto &t : truncate_string_tests) {
    char buffer[1024];
    CopyTruncateStringUTF8({buffer, t.dest_size}, t.src, t.truncate);
    ok1(strcmp(buffer, t.expected_result) == 0);
  }
}

/**
 * Test that CopyString() never produces invalid UTF-8 when
 * truncating.  Each test case specifies a source string, a
 * destination buffer size, and the expected result after
 * UTF-8-safe truncation.
 */
static constexpr struct {
  const char *src;
  size_t dest_size;
  const char *expected_result;
} copy_string_tests[] = {
  /* no truncation needed */
  { "", 10, "" },
  { "abc", 10, "abc" },
  { "abc", 4, "abc" },
  { "\xc3\xbc", 3, "\xc3\xbc" },

  /* ASCII truncation (no UTF-8 issue) */
  { "abcd", 4, "abc" },
  { "abcd", 2, "a" },
  { "abcd", 1, "" },

  /* 2-byte sequence: ü = \xc3\xbc */
  { "foo\xc3\xbc", 6, "foo\xc3\xbc" }, /* fits exactly */
  { "foo\xc3\xbc", 5, "foo" },          /* would cut after \xc3 → crop */
  { "foo\xc3\xbc", 4, "foo" },          /* only room for "foo" */

  /* 3-byte sequence: 目 = \xe7\x9b\xae */
  { "foo\xe7\x9b\xae", 7, "foo\xe7\x9b\xae" }, /* fits exactly */
  { "foo\xe7\x9b\xae", 6, "foo" },               /* cut after 2 of 3 → crop */
  { "foo\xe7\x9b\xae", 5, "foo" },               /* cut after 1 of 3 → crop */
  { "foo\xe7\x9b\xae", 4, "foo" },               /* only room for "foo" */

  /* 4-byte emoji: 😀 = \xf0\x9f\x98\x80 */
  { "a\xf0\x9f\x98\x80", 6, "a\xf0\x9f\x98\x80" }, /* fits */
  { "a\xf0\x9f\x98\x80", 5, "a" },                   /* cut after 3 of 4 */
  { "a\xf0\x9f\x98\x80", 4, "a" },                   /* cut after 2 of 4 */
  { "a\xf0\x9f\x98\x80", 3, "a" },                   /* cut after 1 of 4 */

  /* multiple multi-byte characters */
  { "\xc3\xa4\xc3\xb6\xc3\xbc", 7, "\xc3\xa4\xc3\xb6\xc3\xbc" },
  { "\xc3\xa4\xc3\xb6\xc3\xbc", 6, "\xc3\xa4\xc3\xb6" },  /* cut in 3rd ü */
  { "\xc3\xa4\xc3\xb6\xc3\xbc", 4, "\xc3\xa4" },            /* cut in 2nd ö */
};

static void
TestCopyString()
{
  for (const auto &t : copy_string_tests) {
    char buffer[64];
    CopyString(buffer, t.dest_size, t.src);
    if (!ok1(strcmp(buffer, t.expected_result) == 0))
      diag("CopyString(\"%s\", %zu) = \"%s\", expected \"%s\"",
           t.src, t.dest_size, buffer, t.expected_result);
    /* result must always be valid UTF-8 */
    ok1(ValidateUTF8(buffer));
  }
}

int main()
{
  plan_tests(2 * ARRAY_SIZE(valid) +
             2 * ARRAY_SIZE(invalid) +
             2 * ARRAY_SIZE(length) +
             ARRAY_SIZE(latin1_chars) +
             4 * ARRAY_SIZE(crop) +
             ARRAY_SIZE(truncate_string_tests) +
             2 * ARRAY_SIZE(copy_string_tests) +
             10 + 27);

  for (auto i : valid) {
    ok1(ValidateUTF8(i));
    ok1(LengthUTF8(i) == MyLengthUTF8(i));
  }

  for (auto i : invalid) {
    ok1(!ValidateUTF8(i));
    ok1(!MyValidateUTF8(i));
  }

  for (auto &l : length) {
    ok1(l.length == LengthUTF8(l.value));
    ok1(l.length == MyLengthUTF8(l.value));
  }

  char buffer[64];

  for (auto &l : latin1_chars) {
    *Latin1ToUTF8(l.ch, buffer) = 0;
    ok1(strcmp(l.utf8, buffer) == 0);
  }

  for (auto &c : crop) {
    strcpy(buffer, c.input);
    auto *end = CropIncompleteUTF8(buffer);
    ok1(strcmp(c.output, buffer) == 0);
    ok1(end != nullptr);
    ok1(*end == '\0');
    ok1(end == buffer + strlen(buffer));
  }

  TestTruncateString();
  TestCopyString();

  /* test NextUTF8() */
  {
    const char *p = "foo\xe7\x9b\xae";
    auto n = NextUTF8(p);
    ok1(n.first == 'f');
    ok1(n.second == p + 1);

    n = NextUTF8(p + 1);
    ok1(n.first == 'o');
    ok1(n.second == p + 2);

    n = NextUTF8(p + 2);
    ok1(n.first == 'o');
    ok1(n.second == p + 3);

    n = NextUTF8(p + 3);
    ok1(n.first == 30446);
    ok1(n.second == p + 6);

    n = NextUTF8(p + 6);
    ok1(n.first == 0);
    ok1(n.second == nullptr);
  }

  /* test UnicodeToUTF8() */
  {
    buffer[0] = 1;
    ok1(UnicodeToUTF8(0, buffer) == buffer + 1);
    ok1(buffer[0] == 0);

    ok1(UnicodeToUTF8(' ', buffer) == buffer + 1);
    ok1(buffer[0] == ' ');

    ok1(UnicodeToUTF8(0x7f, buffer) == buffer + 1);
    ok1(buffer[0] == 0x7f);

    ok1(UnicodeToUTF8(0xa2, buffer) == buffer + 2);
    ok1(buffer[0] == char(0xc2));
    ok1(buffer[1] == char(0xa2));

    ok1(UnicodeToUTF8(0x6fb3, buffer) == buffer + 3);
    ok1(buffer[0] == char(0xe6));
    ok1(buffer[1] == char(0xbe));
    ok1(buffer[2] == char(0xb3));

    ok1(UnicodeToUTF8(0xffff, buffer) == buffer + 3);
    ok1(buffer[0] == char(0xef));
    ok1(buffer[1] == char(0xbf));
    ok1(buffer[2] == char(0xbf));

    ok1(UnicodeToUTF8(0x10000, buffer) == buffer + 4);
    ok1(buffer[0] == char(0xf0));
    ok1(buffer[1] == char(0x90));
    ok1(buffer[2] == char(0x80));
    ok1(buffer[3] == char(0x80));

    ok1(UnicodeToUTF8(0x10ffff, buffer) == buffer + 4);
    ok1(buffer[0] == char(0xf4));
    ok1(buffer[1] == char(0x8f));
    ok1(buffer[2] == char(0xbf));
    ok1(buffer[3] == char(0xbf));
  }

  return exit_status();
}
