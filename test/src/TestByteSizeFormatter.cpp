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

#include "Formatter/ByteSizeFormatter.hpp"
#include "Util/Macros.hpp"
#include "Util/StringAPI.hxx"
#include "TestUtil.hpp"

int
main(int argc, char **argv)
{
  plan_tests(31);

  TCHAR buffer[256];

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 0);
  ok1(StringIsEqual(buffer, _T("0 B")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 1);
  ok1(StringIsEqual(buffer, _T("1 B")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 9);
  ok1(StringIsEqual(buffer, _T("9 B")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 10);
  ok1(StringIsEqual(buffer, _T("10 B")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 345);
  ok1(StringIsEqual(buffer, _T("345 B")));


  FormatByteSize(buffer, ARRAY_SIZE(buffer), 1 * 1024);
  ok1(StringIsEqual(buffer, _T("1.00 KB")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 1 * 1024 + 512);
  ok1(StringIsEqual(buffer, _T("1.50 KB")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 9 * 1024);
  ok1(StringIsEqual(buffer, _T("9.00 KB")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 10 * 1024);
  ok1(StringIsEqual(buffer, _T("10.0 KB")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 345 * 1024);
  ok1(StringIsEqual(buffer, _T("345 KB")));


  FormatByteSize(buffer, ARRAY_SIZE(buffer), 1 * 1024 * 1024);
  ok1(StringIsEqual(buffer, _T("1.00 MB")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 1 * 1024 * 1024 + 512 * 1024);
  ok1(StringIsEqual(buffer, _T("1.50 MB")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 9 * 1024 * 1024);
  ok1(StringIsEqual(buffer, _T("9.00 MB")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 10 * 1024 * 1024);
  ok1(StringIsEqual(buffer, _T("10.0 MB")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 345 * 1024 * 1024);
  ok1(StringIsEqual(buffer, _T("345 MB")));


  FormatByteSize(buffer, ARRAY_SIZE(buffer), 1 * 1024 * 1024 * 1024);
  ok1(StringIsEqual(buffer, _T("1.00 GB")));


  FormatByteSize(buffer, ARRAY_SIZE(buffer), 0, true);
  ok1(StringIsEqual(buffer, _T("0B")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 1, true);
  ok1(StringIsEqual(buffer, _T("1B")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 10, true);
  ok1(StringIsEqual(buffer, _T("10B")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 345, true);
  ok1(StringIsEqual(buffer, _T("345B")));


  FormatByteSize(buffer, ARRAY_SIZE(buffer), 1 * 1024, true);
  ok1(StringIsEqual(buffer, _T("1.0K")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 1 * 1024 + 512, true);
  ok1(StringIsEqual(buffer, _T("1.5K")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 9 * 1024, true);
  ok1(StringIsEqual(buffer, _T("9.0K")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 10 * 1024, true);
  ok1(StringIsEqual(buffer, _T("10.0K")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 345 * 1024, true);
  ok1(StringIsEqual(buffer, _T("345K")));


  FormatByteSize(buffer, ARRAY_SIZE(buffer), 1 * 1024 * 1024, true);
  ok1(StringIsEqual(buffer, _T("1.0M")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 1 * 1024 * 1024 + 512 * 1024, true);
  ok1(StringIsEqual(buffer, _T("1.5M")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 9 * 1024 * 1024, true);
  ok1(StringIsEqual(buffer, _T("9.0M")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 10 * 1024 * 1024, true);
  ok1(StringIsEqual(buffer, _T("10.0M")));

  FormatByteSize(buffer, ARRAY_SIZE(buffer), 345 * 1024 * 1024, true);
  ok1(StringIsEqual(buffer, _T("345M")));


  FormatByteSize(buffer, ARRAY_SIZE(buffer), 1 * 1024 * 1024 * 1024, true);
  ok1(StringIsEqual(buffer, _T("1.0G")));


  return exit_status();
}
