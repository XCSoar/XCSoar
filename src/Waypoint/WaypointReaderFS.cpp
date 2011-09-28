/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "WaypointReaderFS.hpp"
#include "Units/Units.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Geo/UTM.hpp"
#include "IO/LineReader.hpp"

#include <stdio.h>

static bool
ParseAngle(const TCHAR *src, Angle &angle)
{
  bool is_positive;
  if (src[0] == _T('N') || src[0] == _T('n') ||
      src[0] == _T('E') || src[0] == _T('e'))
    is_positive = true;
  else if (src[0] == _T('S') || src[0] == _T('s') ||
           src[0] == _T('W') || src[0] == _T('w'))
    is_positive = false;
  else
    return false;

  TCHAR *endptr;

  src++;
  long deg = _tcstol(src, &endptr, 10);
  if (endptr == src || *endptr != _T(' '))
    return false;

  src = endptr;
  long min = _tcstol(src, &endptr, 10);
  if (endptr == src || *endptr != _T(' '))
    return false;

  src = endptr;
  double sec = _tcstod(src, &endptr);
  if (endptr == src || *endptr != _T(' '))
    return false;

  fixed value = fixed(deg + (double)min / 60 + sec / 3600);
  if (!is_positive)
    value = -value;

  angle = Angle::Degrees(value);
  return true;
}

static bool
ParseLocation(const TCHAR *src, GeoPoint &p)
{
  Angle lon, lat;

  if (!ParseAngle(src, lat))
    return false;

  if (!ParseAngle(src + 17, lon))
    return false;

  p.longitude = lon;
  p.latitude = lat;

  // ensure longitude is within -180:180
  p.Normalize();

  return true;
}

static bool
ParseLocationUTM(const TCHAR *src, GeoPoint &p)
{
  TCHAR *endptr;

  long zone_number = _tcstol(src, &endptr, 10);
  if (endptr == src)
    return false;

  src = endptr;
  char zone_letter = src[0];

  src++;
  long easting = _tcstol(src, &endptr, 10);
  if (endptr == src || *endptr != _T(' '))
    return false;

  src = endptr;
  long northing = _tcstol(src, &endptr, 10);
  if (endptr == src || *endptr != _T(' '))
    return false;

  UTM u(zone_number, zone_letter, fixed(easting), fixed(northing));
  p = u.ToGeoPoint();

  // ensure longitude is within -180:180
  p.Normalize();

  return true;
}

static bool
ParseAltitude(const TCHAR *src, fixed &dest)
{
  TCHAR *endptr;
  long alt = _tcstol(src, &endptr, 10);
  if (endptr == src)
    return false;

  dest = fixed(alt);
  return true;
}

static bool
ParseString(const TCHAR *src, tstring &dest, unsigned len = 0)
{
  if (src[0] == 0)
    return true;

  dest.assign(src);
  if (len > 0)
    dest = dest.substr(0, len);

  trim_inplace(dest);

  return true;
}

bool
WaypointReaderFS::ParseLine(const TCHAR* line, const unsigned linenum,
                              Waypoints &way_points)
{
  //$FormatGEO
  //ACONCAGU  S 32 39 12.00    W 070 00 42.00  6962  Aconcagua
  //BERGNEUS  N 51 03 07.02    E 007 42 22.02   488  Bergneustadt [A]
  //GOLDENGA  N 37 49 03.00    W 122 28 42.00   227  Golden Gate Bridge
  //REDSQUAR  N 55 45 15.00    E 037 37 12.00   123  Red Square
  //SYDNEYOP  S 33 51 25.02    E 151 12 54.96     5  Sydney Opera

  //$FormatUTM
  //Aconcagu 19H   0405124   6386692   6962  Aconcagua
  //Bergneus 32U   0409312   5656398    488  Bergneustadt [A]
  //Golden G 10S   0545914   4185695    227  Golden Gate Bridge
  //Red Squa 37U   0413390   6179582    123  Red Square
  //Sydney O 56H   0334898   6252272      5  Sydney Opera

  if (line[0] == '\0')
    return true;

  if (linenum == 0 &&
      _tcsstr(line, _T("$FormatUTM")) == line) {
    is_utm = true;
    return true;
  }

  if (line[0] == _T('$'))
    return true;

  // Determine the length of the line
  size_t len = _tcslen(line);
  // If less then 27 characters -> something is wrong -> cancel
  if (len < (is_utm ? 39 : 47))
    return false;

  GeoPoint location;
  if ((!is_utm && !ParseLocation(line + 10, location)) ||
      (is_utm && !ParseLocationUTM(line + 9, location)))
    return false;

  Waypoint new_waypoint(location);
  new_waypoint.file_num = file_num;
  new_waypoint.original_id = 0;

  if (!ParseString(line, new_waypoint.name, 8))
    return false;

  if (!ParseAltitude(line + (is_utm ? 32 : 41), new_waypoint.altitude))
    CheckAltitude(new_waypoint);

  // Description (Characters 35-44)
  if (len > (is_utm ? 38 : 47))
    ParseString(line + (is_utm ? 38 : 47), new_waypoint.comment);

  way_points.append(new_waypoint);
  return true;
}

bool
WaypointReaderFS::VerifyFormat(TLineReader &reader) const
{
  TCHAR* line = reader.read();
  if (line == NULL)
    return false;

  return (_tcsstr(line, _T("$FormatUTM")) == line ||
          _tcsstr(line, _T("$FormatGEO")) == line);
}
