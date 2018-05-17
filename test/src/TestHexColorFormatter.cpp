/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Formatter/HexColor.hpp"
#include "Screen/PortableColor.hpp"
#include "Util/Macros.hpp"
#include "Util/StringAPI.hxx"
#include "TestUtil.hpp"

int
main(int argc, char **argv)
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
