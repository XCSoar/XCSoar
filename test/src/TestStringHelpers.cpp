// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/StringCompare.hxx"
#include "util/StringStrip.hxx"
#include "TestUtil.hpp"

#include <string.h>

int
main()
{
  plan_tests(34);

  /* StringIsEqualIgnoreCase(std::string_view) */
  ok1(StringIsEqualIgnoreCase("a", "A"));
  ok1(StringIsEqualIgnoreCase("", ""));
  ok1(!StringIsEqualIgnoreCase("a", ""));
  ok1(!StringIsEqualIgnoreCase("", "A"));
  ok1(!StringIsEqualIgnoreCase("aa", "a"));
  ok1(!StringIsEqualIgnoreCase("a", "aa"));
  ok1(StringIsEqualIgnoreCase("AbC", "aBc"));

  /* StringStartsWithIgnoreCase() */
  ok1(StringStartsWithIgnoreCase("foobar", "FOO"));
  ok1(StringStartsWithIgnoreCase("foo", "FOO"));
  ok1(StringStartsWithIgnoreCase("foo", ""));
  ok1(!StringStartsWithIgnoreCase("", "FOO"));
  ok1(!StringStartsWithIgnoreCase("bar", "FOO"));

  /* StringAfterPrefixIgnoreCase() */
  ok1(StringIsEqual(StringAfterPrefixIgnoreCase("foobar", "FOO"), "bar"));
  ok1(StringIsEqual(StringAfterPrefixIgnoreCase("foo", "FOO"), ""));
  ok1(StringAfterPrefixIgnoreCase("bar", "FOO") == nullptr);

  /* StringEndsWith() / StringEndsWithIgnoreCase() */
  ok1(StringEndsWith("foobar", "bar"));
  ok1(StringEndsWith("foobar", ""));
  ok1(StringEndsWith("", ""));
  ok1(!StringEndsWith("foo", "foobar"));
  ok1(StringEndsWithIgnoreCase("FOOBAR", "bar"));
  ok1(StringEndsWithIgnoreCase("foobar", "BAR"));
  ok1(!StringEndsWithIgnoreCase("foo", "BAR"));

  /* FindStringSuffix() */
  {
    const char *p = "foobar";
    ok1(FindStringSuffix(p, "bar") == p + 3);
    ok1(FindStringSuffix(p, "") == p + strlen(p));
    ok1(FindStringSuffix(p, "foobar") == p);
    ok1(FindStringSuffix(p, "baz") == nullptr);
    ok1(FindStringSuffix("foo", "foobar") == nullptr);
  }

  /* StripLeft(const char *) */
  {
    ok1(StringIsEqual(StripLeft(""), ""));
    ok1(StringIsEqual(StripLeft("   \t"), ""));
    ok1(StringIsEqual(StripLeft(" \tfoo"), "foo"));
  }

  /* StripRight(char *) */
  {
    char buffer[32];
    strcpy(buffer, "");
    StripRight(buffer);
    ok1(StringIsEqual(buffer, ""));

    strcpy(buffer, " \t");
    StripRight(buffer);
    ok1(StringIsEqual(buffer, ""));

    strcpy(buffer, "foo \t");
    StripRight(buffer);
    ok1(StringIsEqual(buffer, "foo"));
  }

  /* Strip(char *) */
  {
    char buffer[32];
    strcpy(buffer, "  foo \t");
    ok1(StringIsEqual(Strip(buffer), "foo"));
  }

  return exit_status();
}

