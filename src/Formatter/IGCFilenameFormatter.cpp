// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IGCFilenameFormatter.hpp"
#include "time/BrokenDate.hpp"
#include "util/StringFormat.hpp"

#include <cassert>
#include <string.h>

static char
NumToIGCChar(unsigned num)
{
  assert(num <= 35);

  if (num < 10)
    return _T('1') + (num - 1);

  return _T('A') + (num - 10);
}

void
FormatIGCFilename(char* buffer, const BrokenDate &date,
                  char manufacturer, const char *logger_id,
                  unsigned flight_number)
{
  assert(logger_id != NULL);
  assert(strlen(logger_id) == 3);

  char cyear = NumToIGCChar(date.year % 10);
  char cmonth = NumToIGCChar(date.month);
  char cday = NumToIGCChar(date.day);
  char cflight = NumToIGCChar(flight_number);

  StringFormatUnsafe(buffer, _T("%c%c%c%c%s%c.igc"),
                     cyear, cmonth, cday,
                     manufacturer, logger_id, cflight);
}

void
FormatIGCFilenameLong(char* buffer, const BrokenDate &date,
                      const char *manufacturer, const char *logger_id,
                      unsigned flight_number)
{
  // 2003-12-31-XYZ-987-01.igc
  // long filename form of IGC file.
  // XYZ represents manufacturer code

  assert(manufacturer != NULL);
  assert(strlen(manufacturer) == 3);

  assert(logger_id != NULL);
  assert(strlen(logger_id) == 3);

  StringFormatUnsafe(buffer, _T("%04u-%02u-%02u-%s-%s-%02u.igc"),
                     date.year, date.month, date.day,
                     manufacturer, logger_id, flight_number);
}
