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

#include "WayPointFileWinPilot.hpp"
#include "Units.hpp"
#include "Waypoint/Waypoints.hpp"
#include "IO/TextWriter.hpp"

#include <stdlib.h>

bool
WayPointFileWinPilot::parseLine(const TCHAR* line, const unsigned linenum,
                                Waypoints &way_points, 
                                const RasterTerrain *terrain)
{
  TCHAR ctemp[255];
  const TCHAR *params[20];
  size_t n_params;

  // If (end-of-file or comment)
  if (line[0] == '\0' || line[0] == 0x1a ||
      _tcsstr(line, _T("**")) == line ||
      _tcsstr(line, _T("*")) == line)
    // -> return without error condition
    return true;

  if (_tcslen(line) >= sizeof(ctemp) / sizeof(ctemp[0]))
    /* line too long for buffer */
    return false;

  GeoPoint location;

  // Get fields
  n_params = extractParameters(line, ctemp, params, 20);
  if (n_params < 6)
    return false;

  // Latitude (e.g. 51:15.900N)
  if (!parseAngle(params[1], location.Latitude, true))
    return false;

  // Longitude (e.g. 00715.900W)
  if (!parseAngle(params[2], location.Longitude, false))
    return false;

  Waypoint new_waypoint(location);
  new_waypoint.FileNum = file_num;

  // Name (e.g. KAMPLI)
  if (!parseString(params[5], new_waypoint.Name))
    return false;

  // Altitude (e.g. 458M)
  /// @todo configurable behaviour
  bool alt_ok = parseAltitude(params[3], new_waypoint.Altitude);
  check_altitude(new_waypoint, terrain, alt_ok);

  if (n_params > 6) {
    // Description (e.g. 119.750 Airport)
    parseString(params[6], new_waypoint.Comment);
  }

  // Waypoint Flags (e.g. AT)
  parseFlags(params[4], new_waypoint.Flags);

  add_waypoint(way_points, new_waypoint);
  return true;
}


bool
WayPointFileWinPilot::parseString(const TCHAR* src, tstring& dest)
{
  // Just assign and trim it
  dest.assign(src);
  trim_inplace(dest);
  return true;
}

bool
WayPointFileWinPilot::parseAngle(const TCHAR* src, Angle& dest, const bool lat)
{
  // Two format variants:
  // 51:47.841N (DD:MM.mmm // the usual)
  // 51:23:45N (DD:MM:SS)

  TCHAR *endptr;

  long deg = _tcstol(src, &endptr, 10);
  if (endptr == src || *endptr != _T(':') || deg < 0)
    return false;

  // Limit angle to +/- 90 degrees for Latitude or +/- 180 degrees for Longitude
  deg = std::min(deg, lat ? 90L : 180L);

  src = endptr + 1;
  long min = _tcstol(src, &endptr, 10);
  if (endptr == src || min < 0 || min >= 60)
    return false;

  src = endptr + 1;
  fixed sec;
  if (*endptr == ':') {
    /* 00..59 */
    long l = _tcstol(src, &endptr, 10);
    if (endptr == src || l < 0 || l >= 60)
        return false;

    sec = fixed(l) / 3600;
  } else if (*endptr == '.') {
    /* 000..999 */
    long l = _tcstol(src, &endptr, 10);
    if (endptr != src + 3 || l < 0 || l >= 1000)
        return false;

    sec = fixed(l) / 60000;
  } else
    return false;

  fixed value = fixed(deg) + fixed(min) / 60 + sec;

  TCHAR sign = *endptr;
  if (sign == 'W' || sign == 'w' || sign == 'S' || sign == 's')
    value = -value;

  // Save angle
  dest = Angle::degrees(value);
  return true;
}

bool
WayPointFileWinPilot::parseAltitude(const TCHAR* src, fixed& dest)
{
  // Parse string
  TCHAR *endptr;
  double value = _tcstod(src, &endptr);
  if (endptr == src)
    return false;

  dest = fixed(value);

  // Convert to system unit if necessary
  TCHAR unit = *endptr;
  if (unit == 'F' || unit == 'f')
    dest = Units::ToSysUnit(dest, unFeet);

  // Save altitude
  return true;
}

bool
WayPointFileWinPilot::parseFlags(const TCHAR* src, WaypointFlags& dest)
{
  // A = Airport
  // T = Turnpoint
  // L = Landable
  // H = Home
  // S = Start point
  // F = Finish point
  // R = Restricted
  // W = Waypoint flag

  for (unsigned i = 0; src[i] != 0; i++) {
    switch (src[i]) {
    case 'A':
    case 'a':
      dest.Airport = true;
      break;
    case 'T':
    case 't':
      dest.TurnPoint = true;
      break;
    case 'L':
    case 'l':
      dest.LandPoint = true;
      break;
    case 'H':
    case 'h':
      dest.Home = true;
      break;
    case 'S':
    case 's':
      dest.StartPoint = true;
      break;
    case 'F':
    case 'f':
      dest.FinishPoint = true;
      break;
    case 'R':
    case 'r':
      dest.Restricted = true;
      break;
    case 'W':
    case 'w':
      dest.WaypointFlag = true;
      break;
    }
  }

  return true;
}

void
WayPointFileWinPilot::saveFile(TextWriter &writer, const Waypoints &way_points)
{
  // Iterate through the waypoint list and save each waypoint
  // into the file defined by fp
  /// @todo JMW: iteration ordered by ID would be preferred
  for (Waypoints::WaypointTree::const_iterator it = way_points.begin();
       it != way_points.end(); it++) {
    const Waypoint& wp = it->get_waypoint();
    if (wp.FileNum == file_num)
      composeLine(writer, wp);
  }
}

void
WayPointFileWinPilot::composeLine(TextWriter &writer, const Waypoint& wp)
{
  // Attach the waypoint id to the output
  writer.printf("%u,", wp.id);
  // Attach the latitude to the output
  composeAngle(writer, wp.Location.Latitude, true);
  writer.write(',');
  // Attach the longitude id to the output
  composeAngle(writer, wp.Location.Longitude, false);
  writer.write(',');
  // Attach the altitude id to the output
  composeAltitude(writer, wp.Altitude);
  writer.write(',');
  // Attach the waypoint flags to the output
  composeFlags(writer, wp.Flags);
  writer.write(',');
  // Attach the waypoint name to the output
  writer.write(wp.Name.c_str());
  writer.write(',');
  // Attach the waypoint description to the output
  writer.writeln(wp.Comment.c_str());
}

void
WayPointFileWinPilot::composeAngle(TextWriter &writer,
                                   const Angle& src, const bool lat)
{
  // Calculate degrees, minutes and seconds
  int deg, min, sec;
  bool is_positive;
  if (lat)
    Units::LatitudeToDMS(src, &deg, &min, &sec, &is_positive);
  else
    Units::LongitudeToDMS(src, &deg, &min, &sec, &is_positive);

  // Save them into the buffer string
  writer.printf(lat ? "%02d:%02d:%02d" : "%03d:%02d:%02d",
            deg, min, sec);

  // Attach the buffer string to the output
  if (lat)
    writer.write(is_positive ? "N" : "S");
  else
    writer.write(is_positive ? "E" : "W");
}

void
WayPointFileWinPilot::composeAltitude(TextWriter &writer, const fixed src)
{
  writer.printf("%dM", (int)src);
}

void
WayPointFileWinPilot::composeFlags(TextWriter &writer,
                                   const WaypointFlags &src)
{
  if (src.Airport)
    writer.write('A');
  if (src.TurnPoint)
    writer.write('T');
  if (src.LandPoint)
    writer.write('L');
  if (src.Home)
    writer.write('H');
  if (src.StartPoint)
    writer.write('S');
  if (src.FinishPoint)
    writer.write('F');
  if (src.Restricted)
    writer.write('R');
  if (src.WaypointFlag)
    writer.write('W');

  // set as turnpoint by default if nothing else
  if (!src.Airport && !src.TurnPoint && !src.LandPoint && !src.Home &&
      !src.StartPoint && !src.FinishPoint && !src.Restricted &&
      !src.WaypointFlag)
    writer.write('T');
}

