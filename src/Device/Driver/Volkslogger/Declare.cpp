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

#include "Internal.hpp"
#include "Device/Port.hpp"
#include "Operation.hpp"
#include "vlapi2.h"
#include "Components.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

#ifdef _UNICODE
#include <windows.h>
#endif

#include <algorithm>

static void
CopyToNarrowBuffer(char *dest, size_t max_size, const TCHAR *src)
{
#ifdef _UNICODE
  size_t src_length = _tcslen(src);
  if (src_length >= max_size)
    src_length = max_size - 1;

  int dest_length = WideCharToMultiByte(CP_ACP, 0, src, src_length,
                                        dest, max_size - 1,
                                        NULL, NULL);
  if (dest_length < 0)
    dest_length = 0;
  dest[dest_length] = 0;
#else
  strncpy(dest, src, max_size - 1);
  dest[max_size - 1] = 0;
#endif
}

static void
CopyWaypoint(VLAPI_DATA::WPT &dest, const Waypoint &src)
{
  CopyToNarrowBuffer(dest.name, sizeof(dest.name), src.Name.c_str());
  dest.lon = src.Location.Longitude.value_degrees();
  dest.lat = src.Location.Latitude.value_degrees();
}

static void
CopyTurnPoint(VLAPI_DATA::DCLWPT &dest, const Declaration::TurnPoint &src)
{
  CopyWaypoint(dest, src.waypoint);

  switch (src.shape) {
  case Declaration::TurnPoint::CYLINDER:
    dest.oztyp = VLAPI_DATA::DCLWPT::OZTYP_CYLSKT;
    dest.lw = dest.rz = src.radius;
    dest.rs = 0;
    break;

  case Declaration::TurnPoint::SECTOR:
    dest.oztyp = VLAPI_DATA::DCLWPT::OZTYP_CYLSKT;
    dest.lw = dest.rs = src.radius;
    dest.rz = 0;
    break;

  case Declaration::TurnPoint::LINE:
    dest.oztyp = VLAPI_DATA::DCLWPT::OZTYP_LINE;
    dest.lw = src.radius;
    dest.rs = dest.rz = 0;
    break;
  }

  /* auto direction */
  dest.ws = 360;
}

static bool
DeclareInner(VLAPI &vl, const Declaration &declaration)
{
  assert(declaration.size() >= 2);

  if (vl.open(1, 20, 1, 38400L) != VLA_ERR_NOERR ||
      vl.read_info() != VLA_ERR_NOERR)
    return false;

  memset(&vl.database, 0, sizeof(vl.database));
  memset(&vl.declaration, 0, sizeof(vl.declaration));

  CopyToNarrowBuffer(vl.declaration.flightinfo.pilot,
		     sizeof(vl.declaration.flightinfo.pilot),
                     declaration.PilotName);

  CopyToNarrowBuffer(vl.declaration.flightinfo.gliderid,
                     sizeof(vl.declaration.flightinfo.gliderid),
                     declaration.AircraftReg);

  CopyToNarrowBuffer(vl.declaration.flightinfo.glidertype,
                     sizeof(vl.declaration.flightinfo.glidertype),
                     declaration.AircraftType);

  const Waypoint *home = way_points.GetHome();
  if (home != NULL)
    CopyWaypoint(vl.declaration.flightinfo.homepoint, *home);

  // start..
  CopyTurnPoint(vl.declaration.task.startpoint,
                declaration.TurnPoints.front());

  // rest of task...
  const unsigned n = std::min(declaration.size() - 2, 12u);
  for (unsigned i = 0; i < n; ++i)
    CopyTurnPoint(vl.declaration.task.turnpoints[i],
                  declaration.TurnPoints[i + 1]);

  // Finish
  CopyTurnPoint(vl.declaration.task.finishpoint,
                declaration.TurnPoints.back());

  vl.declaration.task.nturnpoints = n;

  return vl.write_db_and_declaration() == VLA_ERR_NOERR;
}

bool
VolksloggerDevice::Declare(const Declaration &declaration,
                           OperationEnvironment &env)
{
  if (declaration.size() < 2)
    return false;

  env.SetText(_T("Comms with Volkslogger"));

  port->SetRxTimeout(500);

  // change to IO mode baud rate
  unsigned lLastBaudrate = port->SetBaudrate(9600L);

  VLAPI vl(*port, env);

  bool success = DeclareInner(vl, declaration);

  vl.close(1);

  port->SetBaudrate(lLastBaudrate); // restore baudrate

  return success;
}
