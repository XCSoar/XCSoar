// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/UnescapeCString.hpp"
#include "TestUtil.hpp"

#include <string>

int main()
{
  plan_tests(17);

  ok1(UnescapeCString("\\t") == "\t");
  ok1(UnescapeCString("\\a") == "\a");
  ok1(UnescapeCString("\\b") == "\b");
  ok1(UnescapeCString("\\f") == "\f");
  ok1(UnescapeCString("\\v") == "\v");
  ok1(UnescapeCString("\\'") == "'");
  ok1(UnescapeCString("\\\"") == "\"");
  ok1(UnescapeCString("\\n") == "\n");
  ok1(UnescapeCString("\\r") == "\r");
  ok1(UnescapeCString("\\\\") == "\\");
  ok1(UnescapeCString("\\x41") == "A");
  ok1(UnescapeCString("\\101") == "A");
  ok1(UnescapeCString("trailing\\") == "trailing\\");

  /* octal edge cases: max value, overflow-limited, and followed-by-char */
  ok1(UnescapeCString("\\377") == "\xff");
  ok1(UnescapeCString("\\400") == std::string(1, '\0'));
  {
    std::string expected;
    expected.push_back(static_cast<char>(255));
    expected.push_back('1');
    ok1(UnescapeCString("\\3771") == expected);
  }

  const std::string input = "A\\tB\\aC\\bD\\fE\\vF\\'G\\\"H";
  const std::string expected = "A\tB\aC\bD\fE\vF'G\"H";
  ok1(UnescapeCString(input) == expected);

  return exit_status();
}
