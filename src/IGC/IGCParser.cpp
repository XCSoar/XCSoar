/*
Copyright_License {

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

#include "IGCParser.hpp"
#include "IGCHeader.hpp"
#include "IGCFix.hpp"
#include "IGCExtensions.hpp"
#include "IGCDeclaration.hpp"
#include "Time/BrokenDate.hpp"
#include "Time/BrokenTime.hpp"
#include "Util/CharUtil.hpp"
#include "Util/StringAPI.hxx"

#include <stdlib.h>

/**
 * Character table for base-36.
 */
static constexpr char c36[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/**
 * Convert a 5 digit logger serial to a 3 letter logger id.
 */
static void
ImportDeprecatedLoggerSerial(char id[4], unsigned serial)
{
  id[0] = c36[(serial / 36 / 36) % 36];
  id[1] = c36[(serial / 36) % 36];
  id[2] = c36[serial % 36];
  id[3] = 0;
}

bool
IGCParseHeader(const char *line, IGCHeader &header)
{
  /* sample from CAI302: "ACAM3OV" */
  /* sample from Colibri: "ALXN13103FLIGHT:1" */

  if (line[0] != 'A')
    return false;

  ++line;
  size_t length = strlen(line);
  if (length < 6)
    return false;

  memcpy(header.manufacturer, line, 3);
  header.manufacturer[3] = 0;
  line += 3;

  char *endptr;
  unsigned long serial = strtoul(line, &endptr, 10);
  if (endptr == line + 5) {
    /* deprecated: numeric serial, 5 digits (e.g. from Colibri) */
    ImportDeprecatedLoggerSerial(header.id, serial);
    line = endptr;
  } else {
    memcpy(header.id, line, 3);
    header.id[3] = 0;
    line += 3;
  }

  const char *colon = strchr(line, ':');
  header.flight = colon != NULL
    ? strtoul(colon + 1, NULL, 10)
    : 0;

  return true;
}

bool
IGCParseDateRecord(const char *line, BrokenDate &date)
{
  if (memcmp(line, "HFDTE", 5) != 0)
    return false;

  line += 5;

  char *endptr;
  unsigned long value = strtoul(line, &endptr, 10);
  if (endptr != line + 6)
    return false;

  date.year = 1990 + (value + 10) % 100; /* Y2090 bug! */
  date.month = (value / 100) % 100;
  date.day = value / 10000;

  return date.IsPlausible();
}

static int
ParseTwoDigits(const char *p)
{
  if (!IsDigitASCII(p[0]) || !IsDigitASCII(p[1]))
    return -1;

  return (p[0] - '0') * 10 + (p[1] - '0');
}

static bool
CheckThreeAlphaNumeric(const char *src)
{
  return IsAlphaNumericASCII(src[0]) && IsAlphaNumericASCII(src[1]) &&
    IsAlphaNumericASCII(src[2]);
}

bool
IGCParseExtensions(const char *buffer, IGCExtensions &extensions)
{
  if (*buffer++ != 'I')
    return false;

  int count = ParseTwoDigits(buffer);
  if (count < 0)
    return false;

  buffer += 2;

  extensions.clear();

  while (count-- > 0) {
    const int start = ParseTwoDigits(buffer);
    if (start < 8)
      return false;

    buffer += 2;

    const int finish = ParseTwoDigits(buffer);
    if (finish < start)
      return false;

    buffer += 2;

    if (!CheckThreeAlphaNumeric(buffer))
      return false;

    IGCExtension &x = extensions.append();
    x.start = start;
    x.finish = finish;
    memcpy(x.code, buffer, 3);
    x.code[3] = 0;

    buffer += 3;
  }

  return true;
}

/**
 * Parse an unsigned integer from the given string range
 * (null-termination is not necessary).
 *
 * @param p the string
 * @param end the end of the string
 * @return the result, or -1 on error
 */
static int
ParseUnsigned(const char *p, const char *end)
{
  unsigned value = 0;

  for (; p < end; ++p) {
    if (!IsDigitASCII(*p))
      return -1;

    value = value * 10 + (*p - '0');
  }

  return value;
}

static void
ParseExtensionValue(const char *p, const char *end, int16_t &value_r)
{
  int value = ParseUnsigned(p, end);
  if (value >= 0)
    value_r = value;
}

/**
 * Parse the first #n characters from the input string.  If the string
 * is not long enough, nothing is parsed.  This is used to account for
 * columns that are longer than specified; according to LXNav, this is
 * used for decimal places (which are ignored by this function).
 */
static void
ParseExtensionValueN(const char *p, const char *end, size_t n,
                     int16_t &value_r)
{
  if (n > (size_t)(p - end))
    /* string is too short */
    return;

  int value = ParseUnsigned(p, p + n);
  if (value >= 0)
    value_r = value;
}

bool
IGCParseFix(const char *buffer, const IGCExtensions &extensions, IGCFix &fix)
{
  if (*buffer != 'B')
    return false;

  BrokenTime time;
  if (!IGCParseTime(buffer + 1, time))
    return false;

  char valid_char;
  int gps_altitude, pressure_altitude;

  if (sscanf(buffer + 24, "%c%05d%05d",
             &valid_char, &pressure_altitude, &gps_altitude) != 3)
    return false;

  if (valid_char == 'A')
    fix.gps_valid = true;
  else if (valid_char == 'V')
    fix.gps_valid = false;
  else
    return false;

  fix.gps_altitude = gps_altitude;
  fix.pressure_altitude = pressure_altitude;

  if (!IGCParseLocation(buffer + 7, fix.location))
    return false;

  fix.time = time;

  fix.ClearExtensions();

  const size_t line_length = strlen(buffer);
  for (auto i = extensions.begin(), end = extensions.end(); i != end; ++i) {
    const IGCExtension &extension = *i;
    assert(extension.start > 0);
    assert(extension.finish >= extension.start);

    if (extension.finish > line_length)
      /* exceeds the input line length */
      continue;

    const char *start = buffer + extension.start - 1;
    const char *finish = buffer + extension.finish;

    if (StringIsEqual(extension.code, "ENL"))
      ParseExtensionValue(start, finish, fix.enl);
    else if (StringIsEqual(extension.code, "RPM"))
      ParseExtensionValue(start, finish, fix.rpm);
    else if (StringIsEqual(extension.code, "HDM"))
      ParseExtensionValue(start, finish, fix.hdm);
    else if (StringIsEqual(extension.code, "HDT"))
      ParseExtensionValue(start, finish, fix.hdt);
    else if (StringIsEqual(extension.code, "TRM"))
      ParseExtensionValue(start, finish, fix.trm);
    else if (StringIsEqual(extension.code, "TRT"))
      ParseExtensionValue(start, finish, fix.trt);
    else if (StringIsEqual(extension.code, "GSP"))
      ParseExtensionValueN(start, finish, 3, fix.gsp);
    else if (StringIsEqual(extension.code, "IAS"))
      ParseExtensionValueN(start, finish, 3, fix.ias);
    else if (StringIsEqual(extension.code, "TAS"))
      ParseExtensionValueN(start, finish, 3, fix.tas);
    else if (StringIsEqual(extension.code, "SIU"))
      ParseExtensionValue(start, finish, fix.siu);
  }

  return true;
}

bool
IGCParseLocation(const char *buffer, GeoPoint &location)
{
  unsigned lat_degrees, lat_minutes, lon_degrees, lon_minutes;
  char lat_char, lon_char;

  if (sscanf(buffer, "%02u%05u%c%03u%05u%c",
             &lat_degrees, &lat_minutes, &lat_char,
             &lon_degrees, &lon_minutes, &lon_char) != 6)
    return false;

  if (lat_degrees >= 90 || lat_minutes >= 60000 ||
      (lat_char != 'N' && lat_char != 'S'))
    return false;

  if (lon_degrees >= 180 || lon_minutes >= 60000 ||
      (lon_char != 'E' && lon_char != 'W'))
    return false;

  location.latitude = Angle::Degrees(lat_degrees +
                                     lat_minutes / 60000.);
  if (lat_char == 'S')
    location.latitude.Flip();

  location.longitude = Angle::Degrees(lon_degrees +
                                      lon_minutes / 60000.);
  if (lon_char == 'W')
    location.longitude.Flip();

  return true;
}

bool
IGCParseTime(const char *buffer, BrokenTime &time)
{
  unsigned hour, minute, second;

  if (sscanf(buffer, "%02u%02u%02u", &hour, &minute, &second) != 3)
    return false;

  time = BrokenTime(hour, minute, second);
  return time.IsPlausible();
}

static bool
IGCParseDate(const char *buffer, BrokenDate &date)
{
  unsigned day, month, year;

  if (sscanf(buffer, "%02u%02u%02u", &day, &month, &year) != 3)
    return false;

  date = BrokenDate(year + 2000, month, day);
  return date.IsPlausible();
}

bool
IGCParseDeclarationHeader(const char *line, IGCDeclarationHeader &header)
{
  if (*line != 'C' || strlen(line) < 25)
    return false;

  if (!IGCParseDate(line + 1, header.datetime))
    return false;

  if (!IGCParseTime(line + 7, header.datetime))
    return false;

  if (!IGCParseDate(line + 13, header.flight_date))
    header.flight_date.Clear();

  if (!sscanf(line + 23, "%02u", &header.num_turnpoints) ||
      header.num_turnpoints > 99)
    return false;

  std::copy(line + 19, line + 23, header.task_id);

  header.task_name = line + 25;
  return true;
}

bool
IGCParseDeclarationTurnpoint(const char *line, IGCDeclarationTurnpoint &tp)
{
  if (*line != 'C' || strlen(line) < 18)
    return false;

  if (!IGCParseLocation(line + 1, tp.location))
    return false;

  tp.name = line + 18;
  return true;
}
