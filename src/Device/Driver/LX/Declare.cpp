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

#include "Device/Driver/LX.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "Device/Driver/LX/Protocol.hpp"
#include "Device/Port.hpp"
#include "Operation.hpp"

void
LXDevice::CRCWrite(const void *buff, unsigned size)
{
  port->Write(buff, size);
  crc = LX::calc_crc(buff, size, crc);
}

void
LXDevice::CRCWrite(uint8_t c)
{
  port->Write(c);
  crc = LX::calc_crc_char(c, crc);
}

void
LXDevice::CRCWriteint32(int32_t i)
{
  CRCWrite((uint8_t) ((i>>24) & 0xFF));
  CRCWrite((uint8_t) ((i>>16) & 0xFF));
  CRCWrite((uint8_t) ((i>>8) & 0xFF));
  CRCWrite((uint8_t) (i & 0xFF));
}

void
LXDevice::StartNMEAMode(OperationEnvironment &env)
{
  port->Write(LX::SYN);
  env.Sleep(500);
  port->Write(LX::SYN);
  env.Sleep(500);
  port->Write(LX::SYN);
  env.Sleep(500);
}

bool
LXDevice::StartCommandMode()
{
  LX::Connect(*port);
  LX::Connect(*port);
  return LX::Connect(*port);
}

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
      dest[i] = (char)max((src[i] & 0x7f), 0x20);
    else
      dest[i] = '\x20';
  }
  dest[len-1] = '\0';
}

void
LXDevice::LoadPilotInfo(const Declaration &declaration)
{
  memset((void*)lxDevice_Pilot.unknown1, 0, sizeof(lxDevice_Pilot.unknown1));
  copy_space_padded(lxDevice_Pilot.PilotName, declaration.PilotName,
                    sizeof(lxDevice_Pilot.PilotName));
  copy_space_padded(lxDevice_Pilot.GliderType, declaration.AircraftType,
                    sizeof(lxDevice_Pilot.GliderType));
  copy_space_padded(lxDevice_Pilot.GliderID, declaration.AircraftReg,
                    sizeof(lxDevice_Pilot.GliderID));
  copy_space_padded(lxDevice_Pilot.CompetitionID, declaration.CompetitionId,
                    sizeof(lxDevice_Pilot.CompetitionID));
  memset((void*)lxDevice_Pilot.unknown2, 0, sizeof(lxDevice_Pilot.unknown2));
}

void
LXDevice::WritePilotInfo()
{
  CRCWrite(&lxDevice_Pilot, sizeof(lxDevice_Pilot));
  return;
}

/**
 * Loads LX task structure from XCSoar task structure
 * @param decl  The declaration
 */
bool
LXDevice::LoadTask(const Declaration &declaration)
{
  if (declaration.size() > 10)
    return false;

  if (declaration.size() < 2)
      return false;

  memset((void*)lxDevice_Declaration.unknown1, 0,
          sizeof(lxDevice_Declaration.unknown1));

  if (DeclDate.day > 0 && DeclDate.day < 32
      && DeclDate.month > 0 && DeclDate.month < 13) {
    lxDevice_Declaration.dayinput = (unsigned char)DeclDate.day;
    lxDevice_Declaration.monthinput = (unsigned char)DeclDate.month;
    int iCentury = DeclDate.year / 100; // Todo: if no gps fix, use system time
    iCentury *= 100;
    lxDevice_Declaration.yearinput = (unsigned char)(DeclDate.year - iCentury);
  }
  else {
    lxDevice_Declaration.dayinput = (unsigned char)1;
    lxDevice_Declaration.monthinput = (unsigned char)1;
    lxDevice_Declaration.yearinput = (unsigned char)10;
  }
  lxDevice_Declaration.dayuser = lxDevice_Declaration.dayinput;
  lxDevice_Declaration.monthuser = lxDevice_Declaration.monthinput;
  lxDevice_Declaration.yearuser = lxDevice_Declaration.yearinput;
  lxDevice_Declaration.taskid = 0;
  lxDevice_Declaration.numtps = declaration.size();

  for (unsigned i = 0; i < LX::NUMTPS; i++) {
    if (i == 0) { // takeoff
      lxDevice_Declaration.tptypes[i] = 3;
      lxDevice_Declaration.Latitudes[i] = 0;
      lxDevice_Declaration.Longitudes[i] = 0;
      copy_space_padded(lxDevice_Declaration.WaypointNames[i], _T("TAKEOFF"),
        sizeof(lxDevice_Declaration.WaypointNames[i]));


    } else if (i <= declaration.size()) {
      lxDevice_Declaration.tptypes[i] = 1;
      lxDevice_Declaration.Longitudes[i] =
          (int32_t)(declaration.get_location(i - 1).Longitude.value_degrees()
           * 60000);
      lxDevice_Declaration.Latitudes[i] =
          (int32_t)(declaration.get_location(i - 1).Latitude.value_degrees()
           * 60000);
      copy_space_padded(lxDevice_Declaration.WaypointNames[i],
                        declaration.get_name(i - 1),
                        sizeof(lxDevice_Declaration.WaypointNames[i]));

    } else if (i == declaration.size() + 1) { // landing
      lxDevice_Declaration.tptypes[i] = 2;
      lxDevice_Declaration.Longitudes[i] = 0;
      lxDevice_Declaration.Latitudes[i] = 0;
      copy_space_padded(lxDevice_Declaration.WaypointNames[i], _T("LANDING"),
          sizeof(lxDevice_Declaration.WaypointNames[i]));

    } else { // unused
      lxDevice_Declaration.tptypes[i] = 0;
      lxDevice_Declaration.Longitudes[i] = 0;
      lxDevice_Declaration.Latitudes[i] = 0;
      memset((void*)lxDevice_Declaration.WaypointNames[i], 0, 9);
    }
  }

  return true;
}

/**
 * Writes task structure to LX
 * LX task has max 12 points which include Takeoff,
 * start, tps, finish and landing.
 * Leave takeoff and landing as all 0's.
 * @param decl  The declaration
 */
void
LXDevice::WriteTask()
{
  CRCWrite(&lxDevice_Declaration,
            sizeof(lxDevice_Declaration.unknown1) +
            sizeof(lxDevice_Declaration.dayinput) +
            sizeof(lxDevice_Declaration.monthinput) +
            sizeof(lxDevice_Declaration.yearinput) +
            sizeof(lxDevice_Declaration.dayuser) +
            sizeof(lxDevice_Declaration.monthuser) +
            sizeof(lxDevice_Declaration.yearuser));

  CRCWrite(&lxDevice_Declaration.taskid,
            sizeof(lxDevice_Declaration.taskid));
  CRCWrite((char)lxDevice_Declaration.numtps);

  for (unsigned int i = 0; i < LX::NUMTPS; i++) {
    CRCWrite((char)lxDevice_Declaration.tptypes[i]);
  }
  for (unsigned int i = 0; i < LX::NUMTPS; i++) {
    CRCWriteint32(lxDevice_Declaration.Longitudes[i]);
  }
  for (unsigned int i = 0; i < LX::NUMTPS; i++) {
    CRCWriteint32(lxDevice_Declaration.Latitudes[i]);
  }
  for (unsigned int i = 0; i < LX::NUMTPS; i++) {
    CRCWrite(lxDevice_Declaration.WaypointNames[i],
             sizeof(lxDevice_Declaration.WaypointNames[i]));
  }
  return;
}

void
LXDevice::LoadContestClass(gcc_unused const Declaration &declaration)
{
  copy_space_padded(lxDevice_ContestClass.contest_class, _T(""),
                    sizeof(lxDevice_ContestClass.contest_class));
}

void
LXDevice::WriteContestClass()
{
  CRCWrite(&lxDevice_ContestClass.contest_class,
            sizeof(lxDevice_ContestClass.contest_class));
  return;
}

bool
LXDevice::DeclareInner(const Declaration &declaration,
                       gcc_unused OperationEnvironment &env)
{
  if (!port->SetRxTimeout(2000))
    return false;

  env.SetProgressRange(5);
  env.SetProgressPosition(0);

  if (!StartCommandMode())
      return false;

  env.SetProgressPosition(1);

  LoadPilotInfo(declaration);
  if (!LoadTask(declaration))
    return false;
  LoadContestClass(declaration);

  env.SetProgressPosition(2);

  LX::SendCommand(*port, LX::WRITE_FLIGHT_INFO); // start declaration

  crc = 0xff;
  WritePilotInfo();
  env.SetProgressPosition(3);
  WriteTask();
  port->Write(crc);
  if (!LX::ExpectACK(*port))
    return false;

  env.SetProgressPosition(4);
  crc = 0xff;
  LX::SendCommand(*port, LX::WRITE_CONTEST_CLASS);
  WriteContestClass();
  env.SetProgressPosition(5);
  port->Write(crc);
  return LX::ExpectACK(*port);
}

bool
LXDevice::Declare(const Declaration &declaration, OperationEnvironment &env)
{
  if (declaration.size() < 2 || declaration.size() > 12)
    return false;

  if (!port->StopRxThread())
    return false;

  bool success = DeclareInner(declaration, env);

  StartNMEAMode(env);
  port->SetRxTimeout(0);
  port->StartRxThread();
  return success;
}
