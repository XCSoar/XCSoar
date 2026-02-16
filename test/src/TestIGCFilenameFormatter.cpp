// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Formatter/IGCFilenameFormatter.hpp"
#include "util/StringAPI.hxx"
#include "time/BrokenDate.hpp"
#include "TestUtil.hpp"

static void
TestShort()
{
  char buffer[256];

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
  char buffer[256];

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
  char buffer[256];

  FormatIGCFilename(buffer, BrokenDate(2012, 2, 10), 'T', "ABC", 1);
  ok1(StringIsEqual(buffer, _T("22ATABC1.igc")));

  FormatIGCFilenameLong(buffer, BrokenDate(2012, 2, 10),
                        "XYZ", "ABC", 1);
  ok1(StringIsEqual(buffer, _T("2012-02-10-XYZ-ABC-01.igc")));
}

int main()
{
  plan_tests(8);

  TestShort();
  TestLong();

  TestChar();

  return exit_status();
}
