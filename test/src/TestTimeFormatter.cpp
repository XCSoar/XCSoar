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

#include "Formatter/TimeFormatter.hpp"
#include "Util/Macros.hpp"
#include "Util/StringUtil.hpp"
#include "TestUtil.hpp"

static void
TestHHMM()
{
  TCHAR buffer[256];

  FormatSignedTimeHHMM(buffer, 0);
  ok1(StringIsEqual(buffer, _T("00:00")));

  FormatSignedTimeHHMM(buffer, 1);
  ok1(StringIsEqual(buffer, _T("00:00")));

  FormatSignedTimeHHMM(buffer, 59);
  ok1(StringIsEqual(buffer, _T("00:00")));

  FormatSignedTimeHHMM(buffer, 60);
  ok1(StringIsEqual(buffer, _T("00:01")));

  FormatSignedTimeHHMM(buffer, 60 * 5);
  ok1(StringIsEqual(buffer, _T("00:05")));

  FormatSignedTimeHHMM(buffer, 60 * 59);
  ok1(StringIsEqual(buffer, _T("00:59")));

  FormatSignedTimeHHMM(buffer, 60 * 60);
  ok1(StringIsEqual(buffer, _T("01:00")));

  FormatSignedTimeHHMM(buffer, 60 * 60 * 3 + 60 * 25);
  ok1(StringIsEqual(buffer, _T("03:25")));

  FormatSignedTimeHHMM(buffer, 60 * 60 * 19 + 60 * 47);
  ok1(StringIsEqual(buffer, _T("19:47")));

  FormatSignedTimeHHMM(buffer, -(60 * 59));
  ok1(StringIsEqual(buffer, _T("-00:59")));

  FormatSignedTimeHHMM(buffer, -(60 * 60 * 19 + 60 * 47));
  ok1(StringIsEqual(buffer, _T("-19:47")));
}

static void
TestTwoLines()
{
  TCHAR buffer[256], buffer2[256];

  FormatTimeTwoLines(buffer, buffer2, 0);
  ok1(StringIsEqual(buffer, _T("00:00")));
  ok1(StringIsEqual(buffer2, _T("")));

  FormatTimeTwoLines(buffer, buffer2, 1);
  ok1(StringIsEqual(buffer, _T("00:01")));
  ok1(StringIsEqual(buffer2, _T("")));

  FormatTimeTwoLines(buffer, buffer2, 59);
  ok1(StringIsEqual(buffer, _T("00:59")));
  ok1(StringIsEqual(buffer2, _T("")));

  FormatTimeTwoLines(buffer, buffer2, 60);
  ok1(StringIsEqual(buffer, _T("01:00")));
  ok1(StringIsEqual(buffer2, _T("")));

  FormatTimeTwoLines(buffer, buffer2, 60 * 5);
  ok1(StringIsEqual(buffer, _T("05:00")));
  ok1(StringIsEqual(buffer2, _T("")));

  FormatTimeTwoLines(buffer, buffer2, 60 * 59);
  ok1(StringIsEqual(buffer, _T("59:00")));
  ok1(StringIsEqual(buffer2, _T("")));

  FormatTimeTwoLines(buffer, buffer2, 60 * 60);
  ok1(StringIsEqual(buffer, _T("01:00")));
  ok1(StringIsEqual(buffer2, _T("00")));

  FormatTimeTwoLines(buffer, buffer2, 60 * 60 * 3 + 60 * 25 + 13);
  ok1(StringIsEqual(buffer, _T("03:25")));
  ok1(StringIsEqual(buffer2, _T("13")));

  FormatTimeTwoLines(buffer, buffer2, 60 * 60 * 19 + 60 * 47 + 28);
  ok1(StringIsEqual(buffer, _T("19:47")));
  ok1(StringIsEqual(buffer2, _T("28")));

  FormatTimeTwoLines(buffer, buffer2, -(60 * 59));
  ok1(StringIsEqual(buffer, _T("59:00")));
  ok1(StringIsEqual(buffer2, _T("")));

  FormatTimeTwoLines(buffer, buffer2, -(60 * 60 * 19 + 60 * 47 + 28));
  ok1(StringIsEqual(buffer, _T("19:47")));
  ok1(StringIsEqual(buffer2, _T("28")));
}

int
main(int argc, char **argv)
{
  plan_tests(33);

  TestHHMM();
  TestTwoLines();

  return exit_status();
}
