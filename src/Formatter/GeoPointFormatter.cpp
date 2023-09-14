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
FormatLongitude(Angle longitude, TCHAR *buffer, size_t size,
                CoordinateFormat format)
{
  int dd, mm, ss;

  // Calculate Longitude sign
  TCHAR sign = longitude.IsNegative() ? _T('W') : _T('E');

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
    StringFormat(buffer, size, _T("%03d" ) _T(DEG) _T("%02d'%02d\" %c"),
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
    StringFormat(buffer, size, _T("%03d") _T(DEG) _T("%02d'%04.1f\" %c"),
                 dd, mm, mlong, sign);
    break;

  case CoordinateFormat::DDMM_MMM:
    // Calculate degrees
    dd = (int)mlong;
    // Calculate minutes
    mlong = (mlong - dd) * 60.0;
    // Save the string to the buffer
    StringFormat(buffer, size, _T("%03d") _T(DEG) _T("%06.3f' %c"),
                 dd, mlong, sign);
    break;

  case CoordinateFormat::DD_DDDDD:
    // Save the string to the buffer
    StringFormat(buffer, size, _T("%09.5f" DEG " %c"), mlong, sign);
    break;

  case CoordinateFormat::UTM:
    return false;
  }

  return true;
}

bool
FormatLatitude(Angle latitude, TCHAR *buffer, size_t size,
               CoordinateFormat format)
{
  int dd, mm, ss;

  // Calculate Latitude sign
  TCHAR sign = latitude.IsNegative() ? _T('S') : _T('N');

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
    StringFormat(buffer, size, _T("%02d") _T(DEG) _T("%02d'%02d\" %c"),
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
    StringFormat(buffer, size, _T("%02d") _T(DEG) _T("%02d'%04.1f\" %c"),
                 dd, mm, mlat, sign);
    break;

  case CoordinateFormat::DDMM_MMM:
    // Calculate degrees
    dd = (int)mlat;
    // Calculate minutes
    mlat = (mlat - dd) * 60.0;
    // Save the string to the buffer
    StringFormat(buffer, size, _T("%02d") _T(DEG) _T("%06.3f' %c"),
                 dd, mlat, sign);
    break;

  case CoordinateFormat::DD_DDDDD:
    // Save the string to the buffer
    StringFormat(buffer, size, _T("%08.5f" DEG " %c"), mlat, sign);
    break;

  case CoordinateFormat::UTM:
    return false;
  }

  return true;
}

static TCHAR *
FormatUTM(const GeoPoint &location, TCHAR *buffer, size_t size,
          TCHAR separator = _T(' '))
{
  UTM utm = UTM::FromGeoPoint(location);
  StringFormat(buffer, size, _T("%u%c%c%.0f%c%.0f"),
               utm.zone_number, utm.zone_letter, separator,
               (double)utm.easting, separator,
               (double)utm.northing);
  return buffer;
}

TCHAR *
FormatGeoPoint(const GeoPoint &location, TCHAR *buffer, size_t size,
               CoordinateFormat format, TCHAR separator)
{
  if (format == CoordinateFormat::UTM)
    return FormatUTM(location, buffer, size, separator);

  if (!FormatLatitude(location.latitude, buffer, size, format))
    return nullptr;

  TCHAR *end = buffer + size, *p = buffer + StringLength(buffer);
  if (p >= end)
    return nullptr;

  *p++ = separator;

  if (!FormatLongitude(location.longitude, p, end - p, format))
    return nullptr;

  return buffer;
}
