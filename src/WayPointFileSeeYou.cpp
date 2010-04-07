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

#include "WayPointFileSeeYou.hpp"
#include "Units.hpp"
#include "Waypoint/Waypoints.hpp"
#include "RasterTerrain.h"

#include <stdio.h>

bool
WayPointFileSeeYou::parseLine(const TCHAR* line, const unsigned linenum,
    Waypoints &way_points, const RasterTerrain *terrain)
{
  TCHAR ctemp[255];
  const TCHAR *params[20];
  size_t n_params;

  static unsigned iName = 0, iCode = 1, iCountry = 2;
  static unsigned iLatitude = 3, iLongitude = 4, iElevation = 5;
  static unsigned iStyle = 6, iRWDir = 7, iRWLen = 8;
  static unsigned iFrequency = 9, iDescription = 10;

  static bool ignore_following = false;

  // If (end-of-file or comment)
  if (line[0] == '\0' || line[0] == 0x1a ||
      _tcsstr(line, _T("**")) == line ||
      _tcsstr(line, _T("*")) == line)
    // -> return without error condition
    return true;

  // Parse first line holding field order
  /// @todo linenum == 0 should be the first
  /// (not ignored) line, not just line 0
  if (linenum == 0) {
    // Get fields
    n_params = extractParameters(line, ctemp, params, 20);

    // Iterate through fields and save the field order
    for (unsigned i = 0; i < n_params; i++) {
      const TCHAR* value = params[i];

      if (!_tcscmp(value, _T("name")))
        iName = i;
      else if (!_tcscmp(value, _T("code")))
        iCode = i;
      else if (!_tcscmp(value, _T("country")))
        iCountry = i;
      else if (!_tcscmp(value, _T("lat")))
        iLatitude = i;
      else if (!_tcscmp(value, _T("lon")))
        iLongitude = i;
      else if (!_tcscmp(value, _T("elev")))
        iElevation = i;
      else if (!_tcscmp(value, _T("style")))
        iStyle = i;
      else if (!_tcscmp(value, _T("rwdir")))
        iRWDir = i;
      else if (!_tcscmp(value, _T("rwlen")))
        iRWLen = i;
      else if (!_tcscmp(value, _T("freq")))
        iFrequency = i;
      else if (!_tcscmp(value, _T("desc")))
        iDescription = i;
    }
    ignore_following = false;

    return true;
  }

  // If task marker is reached ignore all following lines
  if (_tcsstr(line, _T("-----Related Tasks-----")) == line)
    ignore_following = true;
  if (ignore_following)
    return true;

  Waypoint new_waypoint = way_points.create_from_file(file_num);

  // Get fields
  n_params = extractParameters(line, ctemp, params, 20);

  // Check if the basic fields are provided
  if (iName >= n_params)
    return false;
  if (iLatitude >= n_params)
    return false;
  if (iLongitude >= n_params)
    return false;

  // Name (e.g. "Some Turnpoint", with quotes)
  if (!parseString(params[iName], new_waypoint.Name))
    return false;

  // Latitude (e.g. 5115.900N)
  if (!parseAngle(params[iLatitude], new_waypoint.Location.Latitude, true))
    return false;

  // Longitude (e.g. 00715.900W)
  if (!parseAngle(params[iLongitude], new_waypoint.Location.Longitude, false))
    return false;

  // Elevation (e.g. 458.0m)
  /// @todo configurable behaviour
  bool alt_ok = parseAltitude(params[iElevation], new_waypoint.Altitude);
  check_altitude(new_waypoint, terrain, alt_ok);

  // Description (e.g. "Some Turnpoint", with quotes)
  /// @todo include frequency and rwdir/len
  if (iDescription < n_params)
    parseString(params[iDescription], new_waypoint.Comment);

  // Style (e.g. 5)
  /// @todo include peaks with peak symbols etc.
  if (iStyle < n_params)
    parseStyle(params[iStyle], new_waypoint.Flags);

  // If the Style attribute did not state that this is an airport
  if (!new_waypoint.Flags.Airport) {
    // -> parse the runway length
    fixed rwlen;
    // Runway length (e.g. 546.0m)
    if (iRWLen < n_params && parseAltitude(params[iRWLen], rwlen)) {
      // If runway length is between 100m and 300m -> landpoint
      if (rwlen > 100 && rwlen <= 300)
        new_waypoint.Flags.LandPoint = true;
      // If runway length is higher then 300m -> airport
      if (rwlen > 300)
        new_waypoint.Flags.Airport = true;
    }
  }

  add_waypoint_if_in_range(way_points, new_waypoint, terrain);
  return true;
}

bool
WayPointFileSeeYou::parseString(const TCHAR* src, tstring& dest)
{
  dest.assign(src);

  // Strip quote characters
  int len = dest.length();
  if ((src[0] == '"' || src[0] == '\'') && len >= 2)
    dest = dest.substr(1, len - 2);

  trim_inplace(dest);
  return true;
}

bool
WayPointFileSeeYou::parseAngle(const TCHAR* src, fixed& dest, const bool lat)
{
  double val;
  char sign = 0;

  // Parse string
  int s =_stscanf(src, _T("%lf%c"), &val, &sign);
  // Hack: the E sign for east is interpreted as exponential sign
  if (!(s == 2 || (s == 1 && sign == 0)))
    return false;

  // Calculate angle
  unsigned minfrac = iround((val - (int)val) * 1000);
  unsigned min = (int)val % 100;
  unsigned deg = ((int)val - min) * 0.01;
  val = deg + ((fixed)min + (fixed)minfrac / 1000) / 60;

  // Limit angle to +/- 90 degrees for Latitude or +/- 180 degrees for Longitude
  val = std::min(val, (lat ? 90.0 : 180.0));

  // Make angle negative if southern/western hemisphere
  if (sign == 'W' || sign == 'w' || sign == 'S' || sign == 's')
    val *= -1;

  // Save angle
  dest = (fixed)val;
  return true;
}

bool
WayPointFileSeeYou::parseAltitude(const TCHAR* src, fixed& dest)
{
  double val;
  char unit;

  // Parse string
  if (_stscanf(src, _T("%lf%c"), &val, &unit) != 2)
    return false;

  // Convert to system unit if necessary
  if (unit == 'F' || unit == 'f')
    val = Units::ToSysUnit(val, unFeet);

  // Save altitude
  dest = (fixed)val;
  return true;
}

bool
WayPointFileSeeYou::parseStyle(const TCHAR* src, WaypointFlags& dest)
{
  // 1 - Normal
  // 2 - AirfieldGrass
  // 3 - Outlanding
  // 4 - GliderSite
  // 5 - AirfieldSolid ...
  unsigned style;

  // Parse string
  if (_stscanf(src, _T("%u"), &style) != 1)
    return false;

  // Update flags
  dest.LandPoint = (style == 3);
  dest.Airport = (style >= 2 && style <= 5);

  return true;
}
