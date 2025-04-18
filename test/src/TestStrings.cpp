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

  ok1(StringIsEqual(_T(""), _T("")));
  ok1(!StringIsEqual(_T("a"), _T("")));
  ok1(!StringIsEqual(_T(""), _T("a")));
  ok1(StringIsEqual(_T("aaa"), _T("aaa")));
  ok1(!StringIsEqual(_T("aaa"), _T("a")));
  ok1(!StringIsEqual(_T("aaa"), _T("bbb")));
  ok1(!StringIsEqual(_T("bbb"), _T("aaa")));

  ok1(StringIsEqual(_T("This is a funny test case!"),
                    _T("This is a funny test case!")));

  // Test StringStartsWith()

  ok1(StringStartsWith(_T(""), _T("")));
  ok1(StringStartsWith(_T("a"), _T("")));
  ok1(!StringStartsWith(_T(""), _T("a")));
  ok1(StringStartsWith(_T("aaa"), _T("aaa")));
  ok1(StringStartsWith(_T("aaa"), _T("a")));
  ok1(!StringStartsWith(_T("bbb"), _T("aaa")));

  // Test StringAfterPrefix()

  ok1(StringIsEqual(StringAfterPrefix(_T(""), _T("")), _T("")));
  ok1(StringIsEqual(StringAfterPrefix(_T("a"), _T("")), _T("a")));
  ok1(StringAfterPrefix(_T(""), _T("a")) == NULL);
  ok1(StringIsEqual(StringAfterPrefix(_T("aaa"), _T("aaa")), _T("")));
  ok1(StringIsEqual(StringAfterPrefix(_T("aaa"), _T("a")), _T("aa")));
  ok1(StringAfterPrefix(_T("bbb"), _T("aaa")) == NULL);
  ok1(StringIsEqual(StringAfterPrefix(_T("This is a funny test case!"),
                                      _T("This is")),
                                      _T(" a funny test case!")));

  return exit_status();
}
