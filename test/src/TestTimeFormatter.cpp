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

#include "Formatter/TimeFormatter.hpp"
#include "Util/Macros.hpp"
#include "Util/StringAPI.hxx"
#include "TestUtil.hpp"

static void
TestFormat()
{
  TCHAR buffer[256];

  FormatTime(buffer, 0);
  ok1(StringIsEqual(buffer, _T("00:00:00")));

  FormatTime(buffer, 1);
  ok1(StringIsEqual(buffer, _T("00:00:01")));

  FormatTime(buffer, 59);
  ok1(StringIsEqual(buffer, _T("00:00:59")));

  FormatTime(buffer, 60);
  ok1(StringIsEqual(buffer, _T("00:01:00")));

  FormatTime(buffer, 60 * 5);
  ok1(StringIsEqual(buffer, _T("00:05:00")));

  FormatTime(buffer, 60 * 59);
  ok1(StringIsEqual(buffer, _T("00:59:00")));

  FormatTime(buffer, 60 * 60);
  ok1(StringIsEqual(buffer, _T("01:00:00")));

  FormatTime(buffer, 60 * 60 * 3 + 60 * 25);
  ok1(StringIsEqual(buffer, _T("03:25:00")));

  FormatTime(buffer, 60 * 60 * 19 + 60 * 47 + 43);
  ok1(StringIsEqual(buffer, _T("19:47:43")));

  FormatTime(buffer, -(60 * 59));
  ok1(StringIsEqual(buffer, _T("-00:59:00")));

  FormatTime(buffer, -(60 * 60 * 19 + 60 * 47 + 43));
  ok1(StringIsEqual(buffer, _T("-19:47:43")));
}

static void
TestFormatLong()
{
  TCHAR buffer[256];

  FormatTimeLong(buffer, 0);
  ok1(StringIsEqual(buffer, _T("00:00:00.000")));

  FormatTimeLong(buffer, 1.123);
  ok1(StringIsEqual(buffer, _T("00:00:01.123")));

  FormatTimeLong(buffer, 59);
  ok1(StringIsEqual(buffer, _T("00:00:59.000")));

  FormatTimeLong(buffer, 60.001);
  ok1(StringIsEqual(buffer, _T("00:01:00.001")));

  FormatTimeLong(buffer, 60 * 5);
  ok1(StringIsEqual(buffer, _T("00:05:00.000")));

  FormatTimeLong(buffer, 60 * 59);
  ok1(StringIsEqual(buffer, _T("00:59:00.000")));

  FormatTimeLong(buffer, 60 * 60);
  ok1(StringIsEqual(buffer, _T("01:00:00.000")));

  FormatTimeLong(buffer, 60 * 60 * 3 + 60 * 25);
  ok1(StringIsEqual(buffer, _T("03:25:00.000")));

  FormatTimeLong(buffer, 60 * 60 * 19 + 60 * 47 + 43.765);
  ok1(StringIsEqual(buffer, _T("19:47:43.765")));

  FormatTimeLong(buffer, -(60 * 59));
  ok1(StringIsEqual(buffer, _T("-00:59:00.000")));

  FormatTimeLong(buffer, -(60 * 60 * 19 + 60 * 47 + 43.765));
  ok1(StringIsEqual(buffer, _T("-19:47:43.765")));
}

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

#include <stdio.h>

static void
TestTwoLines()
{
  TCHAR buffer[256], buffer2[256];

  FormatTimeTwoLines(buffer, buffer2, 0);
  ok1(StringIsEqual(buffer, _T("00'00")));
  ok1(StringIsEqual(buffer2, _T("")));

  FormatTimeTwoLines(buffer, buffer2, 1);
  ok1(StringIsEqual(buffer, _T("00'01")));
  ok1(StringIsEqual(buffer2, _T("")));

  FormatTimeTwoLines(buffer, buffer2, 59);
  ok1(StringIsEqual(buffer, _T("00'59")));
  ok1(StringIsEqual(buffer2, _T("")));

  FormatTimeTwoLines(buffer, buffer2, 60);
  ok1(StringIsEqual(buffer, _T("01'00")));
  ok1(StringIsEqual(buffer2, _T("")));

  FormatTimeTwoLines(buffer, buffer2, 60 * 5);
  ok1(StringIsEqual(buffer, _T("05'00")));
  ok1(StringIsEqual(buffer2, _T("")));

  FormatTimeTwoLines(buffer, buffer2, 60 * 59);
  ok1(StringIsEqual(buffer, _T("59'00")));
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
  ok1(StringIsEqual(buffer, _T("-59'00")));
  ok1(StringIsEqual(buffer2, _T("")));

  FormatTimeTwoLines(buffer, buffer2, -(60 * 60 * 19 + 60 * 47 + 28));
  ok1(StringIsEqual(buffer, _T("-19:47")));
  ok1(StringIsEqual(buffer2, _T("28")));
}

static void
TestSmart(int time, const TCHAR *expected_output1,
          const TCHAR *expected_output2, const TCHAR *expected_output3,
          const TCHAR *expected_output4, const TCHAR *separator = _T(" "))
{
  TCHAR buffer[256];

  FormatTimespanSmart(buffer, time, 1, separator);
  ok1(StringIsEqual(buffer, expected_output1));

  FormatTimespanSmart(buffer, time, 2, separator);
  ok1(StringIsEqual(buffer, expected_output2));

  FormatTimespanSmart(buffer, time, 3, separator);
  ok1(StringIsEqual(buffer, expected_output3));

  FormatTimespanSmart(buffer, time, 4, separator);
  ok1(StringIsEqual(buffer, expected_output4));
}

static void
TestSmart()
{
  TestSmart(0, _T("0 sec"), _T("0 sec"), _T("0 sec"), _T("0 sec"));
  TestSmart(1, _T("1 sec"), _T("1 sec"), _T("1 sec"), _T("1 sec"));
  TestSmart(59, _T("59 sec"), _T("59 sec"), _T("59 sec"), _T("59 sec"));
  TestSmart(60, _T("1 min"), _T("1 min"), _T("1 min"), _T("1 min"));

  TestSmart(60 + 59, _T("1 min"), _T("1 min 59 sec"), _T("1 min 59 sec"),
            _T("1 min 59 sec"));

  TestSmart(60 * 5 + 34, _T("5 min"), _T("5 min 34 sec"), _T("5 min 34 sec"),
            _T("5 min 34 sec"));

  TestSmart(60 * 59, _T("59 min"), _T("59 min"), _T("59 min"), _T("59 min"));
  TestSmart(60 * 60, _T("1 h"), _T("1 h"), _T("1 h"), _T("1 h"));

  TestSmart(60 * 60 * 3 + 60 * 25, _T("3 h"), _T("3 h 25 min"),
            _T("3 h 25 min"), _T("3 h 25 min"));

  TestSmart(60 * 60 * 19 + 60 * 47, _T("19 h"), _T("19 h 47 min"),
            _T("19 h 47 min"), _T("19 h 47 min"));

  TestSmart(60 * 60 * 19 + 47, _T("19 h"), _T("19 h"),
            _T("19 h 0 min 47 sec"), _T("19 h 0 min 47 sec"));

  TestSmart(60 * 60 * 19 + 60 * 47 + 5, _T("19 h"), _T("19 h 47 min"),
            _T("19 h 47 min 5 sec"), _T("19 h 47 min 5 sec"));

  TestSmart(60 * 60 * 24 * 3 + 60 * 60 * 19 + 60 * 47 + 5, _T("3 days"),
            _T("3 days 19 h"), _T("3 days 19 h 47 min"),
            _T("3 days 19 h 47 min 5 sec"));

  TestSmart(-(60 * 60 * 24 * 3 + 60 * 60 * 19 + 60 * 47 + 5), _T("-3 days"),
            _T("-3 days 19 h"), _T("-3 days 19 h 47 min"),
            _T("-3 days 19 h 47 min 5 sec"));
}

int
main(int argc, char **argv)
{
  plan_tests(111);

  TestFormat();
  TestFormatLong();
  TestHHMM();
  TestTwoLines();
  TestSmart();

  return exit_status();
}
