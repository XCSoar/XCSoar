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

#include "Internal.hpp"
#include "Protocol.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Declaration.hpp"
#include "vlapi2.h"
#include "dbbconv.h"
#include "Engine/Waypoint/Waypoint.hpp"

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
                                        nullptr, nullptr);
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
  CopyToNarrowBuffer(dest.name, sizeof(dest.name), src.name.c_str());
  dest.location = src.location;
}

static void
CopyTurnPoint(VLAPI_DATA::DCLWPT &dest, const Declaration::TurnPoint &src)
{
  CopyWaypoint(dest, src.waypoint);

  switch (src.shape) {
  case Declaration::TurnPoint::CYLINDER:
    dest.oztyp = VLAPI_DATA::DCLWPT::OZTYP_CYLSKT;
    dest.rz = src.radius;
    dest.rs = 0;
    break;

  case Declaration::TurnPoint::DAEC_KEYHOLE:
    dest.oztyp = VLAPI_DATA::DCLWPT::OZTYP_CYLSKT;
    dest.lw = dest.rs = 10000;
    dest.rz = 500;
    break;

  case Declaration::TurnPoint::SECTOR:
    dest.oztyp = VLAPI_DATA::DCLWPT::OZTYP_CYLSKT;
    dest.rs = src.radius;
    dest.rz = 0;
    break;

  case Declaration::TurnPoint::LINE:
    dest.oztyp = VLAPI_DATA::DCLWPT::OZTYP_LINE;
    dest.lw = (src.radius*2)/1000; //Linewidth is radius*2 unit for lw is km
    dest.rs = dest.rz = 0;
    break;
  }

  /* auto direction */
  dest.ws = 360;
}

static bool
DeclareInner(Port &port, const unsigned bulkrate,
             const Declaration &declaration, const Waypoint *home,
             OperationEnvironment &env)
{
  assert(declaration.Size() >= 2);

  if (!Volkslogger::ConnectAndFlush(port, env, 20000))
    return false;

  //Clear DECLARATION struct and populate with xcs declaration
  VLAPI_DATA::DECLARATION vl_declaration;
  memset(&vl_declaration, 0, sizeof(vl_declaration));
  CopyToNarrowBuffer(vl_declaration.flightinfo.pilot,
		     sizeof(vl_declaration.flightinfo.pilot),
                     declaration.pilot_name);

  CopyToNarrowBuffer(vl_declaration.flightinfo.gliderid,
                     sizeof(vl_declaration.flightinfo.gliderid),
                     declaration.aircraft_registration);

  CopyToNarrowBuffer(vl_declaration.flightinfo.glidertype,
                     sizeof(vl_declaration.flightinfo.glidertype),
                     declaration.aircraft_type);

  if (home != nullptr)
    CopyWaypoint(vl_declaration.flightinfo.homepoint, *home);

  // start..
  CopyTurnPoint(vl_declaration.task.startpoint,
                declaration.turnpoints.front());

  // rest of task...
  const unsigned n = std::min(declaration.Size() - 2, 12u);
  for (unsigned i = 0; i < n; ++i)
    CopyTurnPoint(vl_declaration.task.turnpoints[i],
                  declaration.turnpoints[i + 1]);

  // Finish
  CopyTurnPoint(vl_declaration.task.finishpoint,
                declaration.turnpoints.back());

  vl_declaration.task.nturnpoints = n;

  //populate DBB structure with database(=block) read from logger
  DBB dbb1;
  if (Volkslogger::ReadDatabase(port, bulkrate, env,
                                dbb1.buffer, sizeof(dbb1.buffer)) <= 0)
    return false;

  //do NOT use the declaration(=fdf) from logger
  memset(dbb1.GetFDF(), 0xff, dbb1.FRM_SIZE);

  dbb1.open_dbb();

  //update declaration section
  vl_declaration.put(&dbb1);

  // and write buffer back into VOLKSLOGGER
  if (!Volkslogger::ConnectAndFlush(port, env, 10000))
    return false;

  const bool success =
    Volkslogger::WriteDatabase(port, env, dbb1.buffer, sizeof(dbb1.buffer));
  Volkslogger::Reset(port, env);
  return success;
}

bool
VolksloggerDevice::Declare(const Declaration &declaration,
                           const Waypoint *home,
                           OperationEnvironment &env)
{
  if (declaration.Size() < 2)
    return false;

  port.StopRxThread();

  // change to IO mode baud rate
  unsigned old_baud_rate = port.GetBaudrate();
  if (old_baud_rate == 9600)
    old_baud_rate = 0;
  else if (old_baud_rate != 0 && !port.SetBaudrate(9600))
    return false;

  bool success = DeclareInner(port, bulkrate, declaration, home, env);

  // restore baudrate
  if (old_baud_rate != 0)
    port.SetBaudrate(old_baud_rate);

  return success;
}
