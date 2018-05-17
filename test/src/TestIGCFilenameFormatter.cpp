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

#include "Formatter/IGCFilenameFormatter.hpp"
#include "Util/StringAPI.hxx"
#include "Time/BrokenDate.hpp"
#include "TestUtil.hpp"

static void
TestShort()
{
  TCHAR buffer[256];

  FormatIGCFilename(buffer, BrokenDate(2012, 2, 10), _T('T'), _T("ABC"), 1);
  ok1(StringIsEqual(buffer, _T("22ATABC1.igc")));

  FormatIGCFilename(buffer, BrokenDate(2010, 1, 1), _T('X'), _T("234"), 15);
  ok1(StringIsEqual(buffer, _T("011X234F.igc")));

  FormatIGCFilename(buffer, BrokenDate(2009, 12, 1), _T('X'), _T("234"), 35);
  ok1(StringIsEqual(buffer, _T("9C1X234Z.igc")));
}

static void
TestLong()
{
  TCHAR buffer[256];

  FormatIGCFilenameLong(buffer, BrokenDate(2012, 2, 10),
                        _T("XYZ"), _T("ABC"), 1);
  ok1(StringIsEqual(buffer, _T("2012-02-10-XYZ-ABC-01.igc")));

  FormatIGCFilenameLong(buffer, BrokenDate(2010, 1, 1),
                        _T("BLA"), _T("234"), 15);
  ok1(StringIsEqual(buffer, _T("2010-01-01-BLA-234-15.igc")));

  FormatIGCFilenameLong(buffer, BrokenDate(2009, 12, 1),
                        _T("S45"), _T("234"), 35);
  ok1(StringIsEqual(buffer, _T("2009-12-01-S45-234-35.igc")));
}

static void
TestChar()
{
  TCHAR buffer[256];

  FormatIGCFilename(buffer, BrokenDate(2012, 2, 10), 'T', "ABC", 1);
  ok1(StringIsEqual(buffer, _T("22ATABC1.igc")));

  FormatIGCFilenameLong(buffer, BrokenDate(2012, 2, 10),
                        "XYZ", "ABC", 1);
  ok1(StringIsEqual(buffer, _T("2012-02-10-XYZ-ABC-01.igc")));
}

int
main(int argc, char **argv)
{
  plan_tests(8);

  TestShort();
  TestLong();

  TestChar();

  return exit_status();
}
