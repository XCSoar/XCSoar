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
#include "NanoDeclare.hpp"
#include "Protocol.hpp"
#include "Device/Declaration.hpp"
#include "OS/ByteOrder.hpp"
#include "Operation/Operation.hpp"
#include "Time/BrokenDate.hpp"

/**
 * fills dest with src and appends spaces to end
 * adds '\0' to end of string resulting in
 * len characters with last char = '\0'
 */
static void
copy_space_padded(char dest[], const TCHAR src[], unsigned int len)
{
  const unsigned slen = _tcslen(src);
  for(unsigned i = 0; i < (len - 1); i++) {
    if (i < slen)
      dest[i] = (char)std::max((src[i] & 0x7f), 0x20);
    else
      dest[i] = '\x20';
  }
  dest[len-1] = '\0';
}

static void
LoadPilotInfo(LX::Pilot &lx_driver_Pilot, const Declaration &declaration)
{
  memset((void*)lx_driver_Pilot.unknown1, 0, sizeof(lx_driver_Pilot.unknown1));
  copy_space_padded(lx_driver_Pilot.PilotName, declaration.pilot_name,
                    sizeof(lx_driver_Pilot.PilotName));
  copy_space_padded(lx_driver_Pilot.GliderType, declaration.aircraft_type,
                    sizeof(lx_driver_Pilot.GliderType));
  copy_space_padded(lx_driver_Pilot.GliderID, declaration.aircraft_registration,
                    sizeof(lx_driver_Pilot.GliderID));
  copy_space_padded(lx_driver_Pilot.CompetitionID, declaration.competition_id,
                    sizeof(lx_driver_Pilot.CompetitionID));
  memset((void*)lx_driver_Pilot.unknown2, 0, sizeof(lx_driver_Pilot.unknown2));
}

gcc_const
static int32_t
AngleToLX(Angle value)
{
  return ToBE32((int32_t)(value.Degrees() * 60000));
}

/**
 * Loads LX task structure from XCSoar task structure
 * @param decl  The declaration
 */
static bool
LoadTask(LX::Declaration &lx_driver_Declaration, const Declaration &declaration)
{
  if (declaration.Size() > 10)
    return false;

  if (declaration.Size() < 2)
      return false;

  memset((void*)lx_driver_Declaration.unknown1, 0,
          sizeof(lx_driver_Declaration.unknown1));

  BrokenDate DeclDate;
  DeclDate.day = 1;
  DeclDate.month = 1;
  DeclDate.year = 2010;

  if (DeclDate.day > 0 && DeclDate.day < 32
      && DeclDate.month > 0 && DeclDate.month < 13) {
    lx_driver_Declaration.dayinput = (unsigned char)DeclDate.day;
    lx_driver_Declaration.monthinput = (unsigned char)DeclDate.month;
    int iCentury = DeclDate.year / 100; // Todo: if no gps fix, use system time
    iCentury *= 100;
    lx_driver_Declaration.yearinput = (unsigned char)(DeclDate.year - iCentury);
  }
  else {
    lx_driver_Declaration.dayinput = (unsigned char)1;
    lx_driver_Declaration.monthinput = (unsigned char)1;
    lx_driver_Declaration.yearinput = (unsigned char)10;
  }
  lx_driver_Declaration.dayuser = lx_driver_Declaration.dayinput;
  lx_driver_Declaration.monthuser = lx_driver_Declaration.monthinput;
  lx_driver_Declaration.yearuser = lx_driver_Declaration.yearinput;
  lx_driver_Declaration.taskid = 0;
  lx_driver_Declaration.numtps = declaration.Size();

  for (unsigned i = 0; i < LX::NUMTPS; i++) {
    if (i == 0) { // takeoff
      lx_driver_Declaration.tptypes[i] = 3;
      lx_driver_Declaration.Latitudes[i] = 0;
      lx_driver_Declaration.Longitudes[i] = 0;
      copy_space_padded(lx_driver_Declaration.WaypointNames[i], _T("TAKEOFF"),
        sizeof(lx_driver_Declaration.WaypointNames[i]));


    } else if (i <= declaration.Size()) {
      lx_driver_Declaration.tptypes[i] = 1;
      lx_driver_Declaration.Longitudes[i] =
        AngleToLX(declaration.GetLocation(i - 1).longitude);
      lx_driver_Declaration.Latitudes[i] =
        AngleToLX(declaration.GetLocation(i - 1).latitude);
      copy_space_padded(lx_driver_Declaration.WaypointNames[i],
                        declaration.GetName(i - 1),
                        sizeof(lx_driver_Declaration.WaypointNames[i]));

    } else if (i == declaration.Size() + 1) { // landing
      lx_driver_Declaration.tptypes[i] = 2;
      lx_driver_Declaration.Longitudes[i] = 0;
      lx_driver_Declaration.Latitudes[i] = 0;
      copy_space_padded(lx_driver_Declaration.WaypointNames[i], _T("LANDING"),
          sizeof(lx_driver_Declaration.WaypointNames[i]));

    } else { // unused
      lx_driver_Declaration.tptypes[i] = 0;
      lx_driver_Declaration.Longitudes[i] = 0;
      lx_driver_Declaration.Latitudes[i] = 0;
      memset((void*)lx_driver_Declaration.WaypointNames[i], 0, 9);
    }
  }

  return true;
}

static void
LoadContestClass(LX::ContestClass &lx_driver_ContestClass,
                 gcc_unused const Declaration &declaration)
{
  copy_space_padded(lx_driver_ContestClass.contest_class, _T(""),
                    sizeof(lx_driver_ContestClass.contest_class));
}

static bool
DeclareInner(Port &port, const Declaration &declaration,
             gcc_unused OperationEnvironment &env)
{
  env.SetProgressRange(5);
  env.SetProgressPosition(0);

  if (!LX::CommandMode(port, env))
      return false;

  if (env.IsCancelled())
    return false;

  env.SetProgressPosition(1);

  LX::Pilot pilot;
  LoadPilotInfo(pilot, declaration);

  LX::Declaration lx_driver_Declaration;
  if (!LoadTask(lx_driver_Declaration, declaration))
    return false;

  LX::ContestClass contest_class;
  LoadContestClass(contest_class, declaration);

  if (env.IsCancelled())
    return false;

  env.SetProgressPosition(2);

  LX::SendCommand(port, LX::WRITE_FLIGHT_INFO); // start declaration

  LX::CRCWriter writer(port);
  writer.Write(&pilot, sizeof(pilot), env);
  env.SetProgressPosition(3);

  if (env.IsCancelled())
    return false;

  writer.Write(&lx_driver_Declaration, sizeof(lx_driver_Declaration), env);
  writer.Flush();
  if (!LX::ExpectACK(port, env))
    return false;

  if (env.IsCancelled())
    return false;

  env.SetProgressPosition(4);
  LX::SendCommand(port, LX::WRITE_CONTEST_CLASS);
  writer.Write(&contest_class, sizeof(contest_class), env);
  env.SetProgressPosition(5);

  writer.Flush();
  return LX::ExpectACK(port, env);
}

bool
LXDevice::Declare(const Declaration &declaration,
                  gcc_unused const Waypoint *home,
                  OperationEnvironment &env)
{
  if (declaration.Size() < 2 || declaration.Size() > 12)
    return false;

  if (IsNano())
    return Nano::Declare(port, declaration, env);

  if (!EnableCommandMode(env))
    return false;

  bool success = DeclareInner(port, declaration, env);

  LX::CommandModeQuick(port, env);

  return success;
}
