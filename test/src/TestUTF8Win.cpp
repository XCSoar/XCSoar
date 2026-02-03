// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/gdi/UTF8Win.hpp"

#include <string_view>

extern "C" {
#include "tap.h"
}

int
main()
{
  plan_tests(8);

  /* Returned size() must equal character count (no null in size) */
  ok1(UTF8ToWide(std::string_view("")).size() == 0);
  ok1(UTF8ToWide(std::string_view("H")).size() == 1);
  ok1(UTF8ToWide(std::string_view("Hi")).size() == 2);
  ok1(UTF8ToWide(std::string_view("Hello")).size() == 5);

  /* Multi-byte UTF-8: one wide char per Unicode code point */
  ok1(UTF8ToWide(std::string_view("\xc3\xbc")).size() == 1);   /* U+00FC */
  ok1(UTF8ToWide(std::string_view("caf\xc3\xa9")).size() == 4); /* café */

  /* c_str() remains null-terminated */
  std::wstring w = UTF8ToWide(std::string_view("x"));
  ok1(w.size() == 1);
  ok1(w.c_str()[0] == L'x' && w.c_str()[1] == L'\0');

  return exit_status();
}
