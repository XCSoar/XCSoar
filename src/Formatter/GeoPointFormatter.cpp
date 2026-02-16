// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/** \file GeoPointFormatter.cpp
 * Provides string formatting for the differing coordinate formats 
 * Precision differs between the formats.
 * From http://en.wikipedia.org/wiki/Decimal_degrees 1 degree is 111.32km,
 * this reduces for longitude as progress further north. Multiplying by
 * square root of two as error could be in both axes gives us the worst
 * cases of:
 *
 *   DD.ddddd   precision of  1.57 m
 *   DD MM.mmm                2.62 m
 *   DD MM SS                43.72 m
 *   DD MM SS.s               4.37 m
 */
 
#include "GeoPointFormatter.hpp"
#include "Units/Units.hpp"
#include "Math/Angle.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/UTM.hpp"
#include "util/StringFormat.hpp"
#include "util/StringAPI.hxx"

bool
FormatLongitude(Angle longitude, char *buffer, size_t size,
                CoordinateFormat format)
{
  int dd, mm, ss;

  // Calculate Longitude sign
  char sign = longitude.IsNegative() ? 'W' : 'E';

  double mlong(longitude.AbsoluteDegrees());

  switch (format) {
  case CoordinateFormat::DDMMSS:
    // Calculate degrees
    dd = (int)mlong;
    // Calculate minutes
    mlong = (mlong - dd) * 60.0;
    mm = (int)(mlong);
    // Calculate seconds
    mlong = (mlong - mm) * 60.0;
    ss = (int)(mlong + 0.5);
    if (ss >= 60) {
      mm++;
      ss -= 60;
    }
    if (mm >= 60) {
      dd++;
      mm -= 60;
    }
    // Save the string to the buffer
    StringFormat(buffer, size, "%03d"  DEG "%02d'%02d\" %c",
                 dd, mm, ss, sign);
    break;

  case CoordinateFormat::DDMMSS_S:
    // Calculate degrees
    dd = (int)mlong;
    // Calculate minutes
    mlong = (mlong - dd) * 60.0;
    mm = (int)(mlong);
    // Calculate seconds
    mlong = (mlong - mm) * 60.0;
    // Save the string to the buffer
    StringFormat(buffer, size, "%03d" DEG "%02d'%04.1f\" %c",
                 dd, mm, mlong, sign);
    break;

  case CoordinateFormat::DDMM_MMM:
    // Calculate degrees
    dd = (int)mlong;
    // Calculate minutes
    mlong = (mlong - dd) * 60.0;
    // Save the string to the buffer
    StringFormat(buffer, size, "%03d" DEG "%06.3f' %c",
                 dd, mlong, sign);
    break;

  case CoordinateFormat::DD_DDDDD:
    // Save the string to the buffer
    StringFormat(buffer, size, "%09.5f" DEG " %c", mlong, sign);
    break;

  case CoordinateFormat::UTM:
    return false;
  }

  return true;
}

bool
FormatLatitude(Angle latitude, char *buffer, size_t size,
               CoordinateFormat format)
{
  int dd, mm, ss;

  // Calculate Latitude sign
  char sign = latitude.IsNegative() ? 'S' : 'N';

  double mlat(latitude.AbsoluteDegrees());

  switch (format) {
  case CoordinateFormat::DDMMSS:
    // Calculate degrees
    dd = (int)mlat;
    // Calculate minutes
    mlat = (mlat - dd) * 60.0;
    mm = (int)(mlat);
    // Calculate seconds
    mlat = (mlat - mm) * 60.0;
    ss = (int)(mlat + 0.5);
    if (ss >= 60) {
      mm++;
      ss -= 60;
    }
    if (mm >= 60) {
      dd++;
      mm -= 60;
    }
    // Save the string to the buffer
    StringFormat(buffer, size, "%02d" DEG "%02d'%02d\" %c",
                 dd, mm, ss, sign);
    break;

  case CoordinateFormat::DDMMSS_S:
    // Calculate degrees
    dd = (int)mlat;
    // Calculate minutes
    mlat = (mlat - dd) * 60.0;
    mm = (int)(mlat);
    // Calculate seconds
    mlat = (mlat - mm) * 60.0;
    // Save the string to the buffer
    StringFormat(buffer, size, "%02d" DEG "%02d'%04.1f\" %c",
                 dd, mm, mlat, sign);
    break;

  case CoordinateFormat::DDMM_MMM:
    // Calculate degrees
    dd = (int)mlat;
    // Calculate minutes
    mlat = (mlat - dd) * 60.0;
    // Save the string to the buffer
    StringFormat(buffer, size, "%02d" DEG "%06.3f' %c",
                 dd, mlat, sign);
    break;

  case CoordinateFormat::DD_DDDDD:
    // Save the string to the buffer
    StringFormat(buffer, size, "%08.5f" DEG " %c", mlat, sign);
    break;

  case CoordinateFormat::UTM:
    return false;
  }

  return true;
}

static char *
FormatUTM(const GeoPoint &location, char *buffer, size_t size,
          char separator = ' ')
{
  UTM utm = UTM::FromGeoPoint(location);
  StringFormat(buffer, size, "%u%c%c%.0f%c%.0f",
               utm.zone_number, utm.zone_letter, separator,
               (double)utm.easting, separator,
               (double)utm.northing);
  return buffer;
}

char *
FormatGeoPoint(const GeoPoint &location, char *buffer, size_t size,
               CoordinateFormat format, char separator)
{
  if (format == CoordinateFormat::UTM)
    return FormatUTM(location, buffer, size, separator);

  if (!FormatLatitude(location.latitude, buffer, size, format))
    return nullptr;

  char *end = buffer + size, *p = buffer + StringLength(buffer);
  if (p >= end)
    return nullptr;

  *p++ = separator;

  if (!FormatLongitude(location.longitude, p, end - p, format))
    return nullptr;

  return buffer;
}
