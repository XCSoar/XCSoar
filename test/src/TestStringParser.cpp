// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/StringCompare.hxx"
#include "util/StringParser.hxx"
#include "TestUtil.hpp"

#include <math.h>

int
main()
{
  plan_tests(30);

  /* Strip() */
  {
    StringParser p{"  \tfoo"};
    p.Strip();
    ok1(p.MatchAll("foo"));
  }

  /* Match()/SkipMatch() */
  {
    StringParser p{"hello world"};
    ok1(p.Match("hello"));
    ok1(p.SkipMatch("hello"));
    ok1(p.Match(' '));
    ok1(p.SkipMatch(' '));
    ok1(p.MatchAll("world"));
  }

  /* MatchIgnoreCase()/SkipMatchIgnoreCase() */
  {
    StringParser p{"FoOBar"};
    ok1(p.MatchIgnoreCase("foo"));
    ok1(p.SkipMatchIgnoreCase("fOo"));
    ok1(p.MatchIgnoreCase("bar"));
    ok1(p.SkipMatchIgnoreCase("BAR"));
    ok1(p.IsEmpty());
  }

  /* ReadUnsigned() basic */
  {
    StringParser p{"123x"};
    const auto value = p.ReadUnsigned();
    ok1(value.has_value());
    ok1(*value == 123);
    ok1(p.MatchAll("x"));
  }

  /* ReadUnsigned() base 16 */
  {
    StringParser p{"ff "};
    const auto value = p.ReadUnsigned(16);
    ok1(value.has_value());
    ok1(*value == 255);
    ok1(p.Match(' '));
  }

  /* ReadUnsigned() failure must not advance */
  {
    StringParser p{"x123"};
    const auto value = p.ReadUnsigned();
    ok1(!value.has_value());
    ok1(p.MatchAll("x123"));
  }

  /* ReadDouble() basic */
  {
    StringParser p{"-1.25,"};
    const auto value = p.ReadDouble();
    ok1(value.has_value());
    ok1(fabs(*value - (-1.25)) < 1e-12);
    ok1(p.Match(','));
  }

  /* ReadDouble() failure must not advance */
  {
    StringParser p{"foo"};
    const auto value = p.ReadDouble();
    ok1(!value.has_value());
    ok1(p.MatchAll("foo"));
  }

  /* SkipWord() */
  {
    StringParser p{"one two\tthree"};
    ok1(p.SkipWord());
    ok1(p.MatchAll("two\tthree"));
    ok1(p.SkipWord());
    ok1(p.MatchAll("three"));
    ok1(!p.SkipWord());
    ok1(p.MatchAll(""));
  }

  return exit_status();
}

