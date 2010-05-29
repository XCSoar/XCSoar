/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "WayPointFileZander.hpp"
#include "Units.hpp"
#include "Waypoint/Waypoints.hpp"

#include <stdio.h>

bool
WayPointFileZander::parseLine(const TCHAR* line, const unsigned linenum,
                              Waypoints &way_points,
                              const RasterTerrain *terrain)
{
  // If (end-of-file or comment)
  if (line[0] == '\0' || line[0] == 0x1a ||
      _tcsstr(line, _T("**")) == line ||
      _tcsstr(line, _T("*")) == line)
    // -> return without error condition
    return true;

  // Determine the length of the line
  size_t len = _tcslen(line);
  // If less then 34 characters -> something is wrong -> cancel
  if (len < 34)
    return false;

  GEOPOINT location;

  // Latitude (Characters 13-20 // DDMMSS(N/S))
  if (!parseAngle(line + 13, location.Latitude, true))
    return false;

  // Longitude (Characters 21-29 // DDDMMSS(E/W))
  if (!parseAngle(line + 21, location.Longitude, false))
    return false;

  Waypoint new_waypoint(location);
  new_waypoint.FileNum = file_num;

  // Name (Characters 0-12)
  if (!parseString(line, new_waypoint.Name))
    return false;

  // Altitude (Characters 30-34 // e.g. 1561 (in meters))
  /// @todo configurable behaviour
  bool alt_ok = parseAltitude(line + 30, new_waypoint.Altitude);
  check_altitude(new_waypoint, terrain, alt_ok);

  // Description (Characters 35-44)
  if (len > 35)
    parseString(line + 35, new_waypoint.Comment);

  // Flags (Characters 45-49)
  if (len > 45)
    parseFlags(line + 45, new_waypoint.Flags);

  add_waypoint_if_in_range(way_points, new_waypoint, terrain);
  return true;
}


bool
WayPointFileZander::parseString(const TCHAR* src, tstring& dest)
{
  if (src[0] == 0)
    return true;

  dest.assign(src);

  // Cut the string after the first space, tab or null character
  size_t found = dest.find_first_of(_T(" \t\0"));
  if (found != tstring::npos)
    dest = dest.substr(0, found);

  trim_inplace(dest);
  return true;
}

bool
WayPointFileZander::parseAngle(const TCHAR* src, Angle& dest, const bool lat)
{
  double val;
  char sign = 0;

  // Parse string
  int s =_stscanf(src, _T("%lf%c"), &val, &sign);
  // Hack: the E sign for east is interpreted as exponential sign
  if (!(s == 2 || (s == 1 && sign == 0)))
    return false;

  // Calculate angle
  unsigned deg = (int)(val * 0.0001);
  unsigned min = (int)((val - deg * 10000) * 0.01);
  unsigned sec = iround((int)val % 100);
  val = fixed(deg) + ((fixed)min / 60) + ((fixed)sec / 3600);

  // Limit angle to +/- 90 degrees for Latitude or +/- 180 degrees for Longitude
  val = std::min(val, (lat ? 90.0 : 180.0));

  // Make angle negative if southern/western hemisphere
  if (sign == 'W' || sign == 'w' || sign == 'S' || sign == 's')
    val *= -1;

  // Save angle
  dest = Angle::degrees((fixed)val);
  return true;
}

bool
WayPointFileZander::parseAltitude(const TCHAR* src, fixed& dest)
{
  double val;

  // Parse string
  if (_stscanf(src, _T("%lf"), &val) != 1)
    return false;

  // Save altitude
  dest = (fixed)val;
  return true;
}

bool
WayPointFileZander::parseFlags(const TCHAR* src, WaypointFlags& dest)
{
  // WP = Waypoint
  // HA = Home Field
  // WA = WP + Airfield
  // LF = Landing Field
  // WL = WP + Landing Field
  // RA = Restricted

  // If flags field exists (this function isn't called otherwise)
  // -> turnpoint needs to be declared as one
  dest.TurnPoint = false;

  if ((src[0] == 'W' || src[0] == 'w') &&
      (src[1] == 'P' || src[1] == 'p')) {
    dest.TurnPoint = true;
  }
  else if ((src[0] == 'H' || src[0] == 'h') &&
           (src[1] == 'A' || src[1] == 'a')) {
    dest.TurnPoint = true;
    dest.Airport = true;
    dest.Home = true;
  }
  else if ((src[0] == 'W' || src[0] == 'w') &&
           (src[1] == 'A' || src[1] == 'a')) {
    dest.TurnPoint = true;
    dest.Airport = true;
  }
  else if ((src[0] == 'L' || src[0] == 'l') &&
           (src[1] == 'F' || src[1] == 'f')) {
    dest.LandPoint = true;
  }
  else if ((src[0] == 'W' || src[0] == 'w') &&
           (src[1] == 'L' || src[1] == 'l')) {
    dest.TurnPoint = true;
    dest.LandPoint = true;
  }
  else if ((src[0] == 'R' || src[0] == 'r') &&
           (src[1] == 'A' || src[1] == 'a')) {
    dest.Restricted = true;
  }

  return true;
}
