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
#include "RasterTerrain.h"
#include "IO/TextWriter.hpp"

bool
WayPointFileWinPilot::parseLine(const TCHAR* line, const unsigned linenum,
                                Waypoints &way_points, const RasterTerrain *terrain)
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

  Waypoint new_waypoint = way_points.create_from_file(file_num);

  // Get fields
  n_params = extractParameters(line, ctemp, params, 20);
  if (n_params < 5)
    return false;

  // Name (e.g. KAMPLI)
  if (!parseString(params[5], new_waypoint.Name))
    return false;

  // Latitude (e.g. 51:15.900N)
  if (!parseAngle(params[1], new_waypoint.Location.Latitude, true))
    return false;

  // Longitude (e.g. 00715.900W)
  if (!parseAngle(params[2], new_waypoint.Location.Longitude, false))
    return false;

  // Altitude (e.g. 458M)
  /// @todo configurable behaviour
  bool alt_ok = parseAltitude(params[3], new_waypoint.Altitude);
  check_altitude(new_waypoint, terrain, alt_ok);

  // Description (e.g. 119.750 Airport)
  parseString(params[6], new_waypoint.Comment);

  // Waypoint Flags (e.g. AT)
  parseFlags(params[4], new_waypoint.Flags);

  add_waypoint_if_in_range(way_points, new_waypoint, terrain);
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

  double val;
  char sign = 0;
  unsigned deg, min, sec;

  // Parse unusual string
  int s =_stscanf(src, _T("%u:%u:%u%c"), &deg, &min, &sec, &sign);
  // Hack: the E sign for east is interpreted as exponential sign
  if (s == 4 || (s == 3 && sign == 0)) {
    // Calculate angle
    val = deg + (fixed)min / 60 + (fixed)sec / 3600;
  } else {
    // Parse usual string
    s =_stscanf(src, _T("%u:%lf%c"), &deg, &val, &sign);
    // Hack: the E sign for east is interpreted as exponential sign
    if (!(s == 3 || (s == 2 && sign == 0)))
      return false;

    // Calculate angle
    unsigned minfrac = iround((val - (int)val) * 1000);
    min = (int)val;
    val = deg + ((fixed)min + (fixed)minfrac / 1000) / 60;
  }

  // Limit angle to +/- 90 degrees for Latitude or +/- 180 degrees for Longitude
  val = std::min(val, (lat ? 90.0 : 180.0));

  // Make angle negative if southern/western hemisphere
  if (sign == 'W' || sign == 'w' || sign == 'S' || sign == 's')
    val *= -1;

  // Save angle
  dest = Angle((fixed)val);
  return true;
}

bool
WayPointFileWinPilot::parseAltitude(const TCHAR* src, fixed& dest)
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
      writer.writeln(composeLine(wp).c_str());
  }
}

tstring
WayPointFileWinPilot::composeLine(const Waypoint& wp)
{
  // Prepare waypoint id
  TCHAR buf[10];
  _stprintf(buf, _T("%u"), wp.id);

  // Attach the waypoint id to the output
  tstring dest = buf;
  dest += _T(",");
  // Attach the latitude to the output
  dest += composeAngle(wp.Location.Latitude, true);
  dest += _T(",");
  // Attach the longitude id to the output
  dest += composeAngle(wp.Location.Longitude, false);
  dest += _T(",");
  // Attach the altitude id to the output
  dest += composeAltitude(wp.Altitude);
  dest += _T(",");
  // Attach the waypoint flags to the output
  dest += composeFlags(wp.Flags);
  dest += _T(",");
  // Attach the waypoint name to the output
  dest += wp.Name;
  dest += _T(",");
  // Attach the waypoint description to the output
  dest += wp.Comment;
  return dest;
}

tstring
WayPointFileWinPilot::composeAngle(const Angle& src, const bool lat)
{
  TCHAR buffer[20];
  bool is_negative = negative(src.value());

  // Calculate degrees, minutes and seconds
  fixed smag = src.magnitude();
  int deg = smag;
  int min = (smag - deg) * 60;
  int sec = (((smag - deg) * 60) - min) * 60;

  // Save them into the buffer string
  _stprintf(buffer, (lat ? _T("%02d:%02d:%02d") : _T("%03d:%02d:%02d")),
            deg, min, sec);

  // Attach the buffer string to the output
  tstring dest = buffer;
  if (lat)
    dest += (is_negative ? _T("S") : _T("N"));
  else
    dest += (is_negative ? _T("W") : _T("E"));

  return dest;
}

tstring
WayPointFileWinPilot::composeAltitude(const fixed& src)
{
  // Save the formatted altitude into the buffer string
  TCHAR buf[10];
  _stprintf(buf, _T("%d"), (int)src);

  // Attach the buffer string to the output string
  tstring dest = buf;
  // Attach the unit to the output string
  dest += _T("M");
  return dest;
}

tstring
WayPointFileWinPilot::composeFlags(const WaypointFlags& src)
{
  tstring dest = _T("");

  if (src.Airport)
    dest += _T("A");
  if (src.TurnPoint)
    dest += _T("T");
  if (src.LandPoint)
    dest += _T("L");
  if (src.Home)
    dest += _T("H");
  if (src.StartPoint)
    dest += _T("S");
  if (src.FinishPoint)
    dest += _T("F");
  if (src.Restricted)
    dest += _T("R");
  if (src.WaypointFlag)
    dest += _T("W");

  // set as turnpoint by default if nothing else
  if (dest.length() < 1)
    dest = _T("T");

  return dest;
}

