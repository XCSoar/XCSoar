// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Protocol.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Declaration.hpp"
#include "vlapi2.h"
#include "dbbconv.h"
#include "Engine/Waypoint/Waypoint.hpp"

#include <algorithm>

static void
CopyToNarrowBuffer(char *dest, size_t max_size, const char *src)
{
  strncpy(dest, src, max_size - 1);
  dest[max_size - 1] = 0;
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

  Volkslogger::ConnectAndFlush(port, env, std::chrono::seconds(20));

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
  Volkslogger::ConnectAndFlush(port, env, std::chrono::seconds(10));

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
  else if (old_baud_rate != 0)
    port.SetBaudrate(9600);

  bool success = DeclareInner(port, bulkrate, declaration, home, env);

  // restore baudrate
  if (old_baud_rate != 0)
    port.SetBaudrate(old_baud_rate);

  return success;
}
