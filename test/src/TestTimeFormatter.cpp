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

  TimeToTextHHMMSigned(buffer, 0);
  ok1(StringIsEqual(buffer, _T("00:00")));

  TimeToTextHHMMSigned(buffer, 1);
  ok1(StringIsEqual(buffer, _T("00:00")));

  TimeToTextHHMMSigned(buffer, 59);
  ok1(StringIsEqual(buffer, _T("00:00")));

  TimeToTextHHMMSigned(buffer, 60);
  ok1(StringIsEqual(buffer, _T("00:01")));

  TimeToTextHHMMSigned(buffer, 60 * 5);
  ok1(StringIsEqual(buffer, _T("00:05")));

  TimeToTextHHMMSigned(buffer, 60 * 59);
  ok1(StringIsEqual(buffer, _T("00:59")));

  TimeToTextHHMMSigned(buffer, 60 * 60);
  ok1(StringIsEqual(buffer, _T("01:00")));

  TimeToTextHHMMSigned(buffer, 60 * 60 * 3 + 60 * 25);
  ok1(StringIsEqual(buffer, _T("03:25")));

  TimeToTextHHMMSigned(buffer, 60 * 60 * 19 + 60 * 47);
  ok1(StringIsEqual(buffer, _T("19:47")));

  TimeToTextHHMMSigned(buffer, -(60 * 59));
  ok1(StringIsEqual(buffer, _T("-00:59")));

  TimeToTextHHMMSigned(buffer, -(60 * 60 * 19 + 60 * 47));
  ok1(StringIsEqual(buffer, _T("-19:47")));
}

int
main(int argc, char **argv)
{
  plan_tests(11);

  TestHHMM();

  return exit_status();
}
