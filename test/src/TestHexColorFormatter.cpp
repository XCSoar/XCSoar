// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Formatter/HexColor.hpp"
#include "ui/canvas/PortableColor.hpp"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"
#include "TestUtil.hpp"

int main()
{
  plan_tests(18);

  char buffer[16];
  RGB8Color color;

  FormatHexColor(buffer, ARRAY_SIZE(buffer), RGB8Color(0x12, 0x34, 0x56));
  ok1(StringIsEqual(buffer, "#123456"));
  ok1(ParseHexColor(buffer, color));
  ok1(color.Red() == 0x12);
  ok1(color.Green() == 0x34);
  ok1(color.Blue() == 0x56);

  FormatHexColor(buffer, ARRAY_SIZE(buffer), RGB8Color(0xff, 0x00, 0xcc));
  ok1(StringIsEqual(buffer, "#FF00CC"));
  ok1(ParseHexColor(buffer, color));
  ok1(color.Red() == 0xFF);
  ok1(color.Green() == 0x00);
  ok1(color.Blue() == 0xCC);

  FormatHexColor(buffer, ARRAY_SIZE(buffer), RGB8Color(0xA4, 0xB9, 0x3C));
  ok1(StringIsEqual(buffer, "#A4B93C"));
  ok1(ParseHexColor(buffer, color));
  ok1(color.Red() == 0xA4);
  ok1(color.Green() == 0xB9);
  ok1(color.Blue() == 0x3C);

  ok1(!ParseHexColor("A4B93C", color));
  ok1(!ParseHexColor("#A4B93", color));
  ok1(!ParseHexColor("#A4B93G", color));

  return exit_status();
}
