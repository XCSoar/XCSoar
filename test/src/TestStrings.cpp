// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "TestUtil.hpp"

#include <tchar.h>

int main()
{
  plan_tests(29);

  // Test StringIsEqual()

  ok1(StringIsEqual("", ""));
  ok1(!StringIsEqual("a", ""));
  ok1(!StringIsEqual("", "a"));
  ok1(StringIsEqual("aaa", "aaa"));
  ok1(!StringIsEqual("aaa", "a"));
  ok1(!StringIsEqual("aaa", "bbb"));
  ok1(!StringIsEqual("bbb", "aaa"));

  ok1(StringIsEqual("This is a funny test case!",
                    "This is a funny test case!"));

  ok1(StringIsEqual("", ""));
  ok1(!StringIsEqual("a", ""));
  ok1(!StringIsEqual("", "a"));
  ok1(StringIsEqual("aaa", "aaa"));
  ok1(!StringIsEqual("aaa", "a"));
  ok1(!StringIsEqual("aaa", "bbb"));
  ok1(!StringIsEqual("bbb", "aaa"));

  ok1(StringIsEqual("This is a funny test case!",
                    "This is a funny test case!"));

  // Test StringStartsWith()

  ok1(StringStartsWith("", ""));
  ok1(StringStartsWith("a", ""));
  ok1(!StringStartsWith("", "a"));
  ok1(StringStartsWith("aaa", "aaa"));
  ok1(StringStartsWith("aaa", "a"));
  ok1(!StringStartsWith("bbb", "aaa"));

  // Test StringAfterPrefix()

  ok1(StringIsEqual(StringAfterPrefix("", ""), ""));
  ok1(StringIsEqual(StringAfterPrefix("a", ""), "a"));
  ok1(StringAfterPrefix("", "a") == NULL);
  ok1(StringIsEqual(StringAfterPrefix("aaa", "aaa"), ""));
  ok1(StringIsEqual(StringAfterPrefix("aaa", "a"), "aa"));
  ok1(StringAfterPrefix("bbb", "aaa") == NULL);
  ok1(StringIsEqual(StringAfterPrefix("This is a funny test case!",
                                      "This is"),
                                      " a funny test case!"));

  return exit_status();
}
