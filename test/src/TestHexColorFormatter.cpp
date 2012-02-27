/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Screen/Color.hpp"
#include "Util/Macros.hpp"
#include "Util/StringUtil.hpp"
#include "TestUtil.hpp"

int
main(int argc, char **argv)
{
  plan_tests(3);

  TCHAR buffer[16];

  FormatHexColor(buffer, ARRAY_SIZE(buffer), Color(0x12, 0x34, 0x56));
  ok1(StringIsEqual(buffer, _T("#123456")));

  FormatHexColor(buffer, ARRAY_SIZE(buffer), Color(0xff, 0x00, 0xcc));
  ok1(StringIsEqual(buffer, _T("#FF00CC")));

  FormatHexColor(buffer, ARRAY_SIZE(buffer), Color(0xA4, 0xB9, 0x3C));
  ok1(StringIsEqual(buffer, _T("#A4B93C")));

  return exit_status();
}
