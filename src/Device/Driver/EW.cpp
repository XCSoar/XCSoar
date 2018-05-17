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


// ToDo

// adding baro alt sentance parser to support baro source priority  if (d == pDevPrimaryBaroSource){...}

#include "Device/Driver/EW.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Declaration.hpp"
#include "NMEA/Checksum.hpp"
#include "Operation/Operation.hpp"
#include "Util/TruncateString.hpp"
#include "Util/ConvertString.hpp"

#include <tchar.h>
#include <stdio.h>
#include "Waypoint/Waypoint.hpp"

#ifdef _UNICODE
#include <windows.h>
#endif

// Additional sentance for EW support

class EWDevice : public AbstractDevice {
protected:
  Port &port;
  unsigned lLastBaudrate;
  int ewDecelTpIndex;

public:
  EWDevice(Port &_port)
    :port(_port),
     lLastBaudrate(0), ewDecelTpIndex(0) {}

protected:
  bool TryConnect(OperationEnvironment &env);
  bool AddWaypoint(const Waypoint &way_point, OperationEnvironment &env);
  bool DeclareInner(const struct Declaration &declaration,
                    OperationEnvironment &env);

public:
  /* virtual methods from class Device */
  void LinkTimeout() override;
  bool Declare(const Declaration &declaration, const Waypoint *home,
               OperationEnvironment &env) override;
};

static void
WriteWithChecksum(Port &port, const char *String)
{
  port.Write(String);

  char sTmp[8];
  sprintf(sTmp, "%02X\r\n", ::NMEAChecksum(String));
  port.Write(sTmp);
}

bool
EWDevice::TryConnect(OperationEnvironment &env)
{
  int retries = 10;
  while (--retries) {

    // send IO Mode command
    port.Write("##\r\n");
    if (port.ExpectString("IO Mode.\r", env))
      return true;

    if (!port.FullFlush(env, 100, 500))
      return false;
  }

  return false;
}

static void
convert_string(char *dest, size_t size, const TCHAR *src)
{
#ifdef _UNICODE
  size_t length = _tcslen(src);
  if (length >= size)
    length = size - 1;

  int length2 = ::WideCharToMultiByte(CP_ACP, 0, src, length, dest, size,
                                      nullptr, nullptr);
  if (length2 < 0)
    length2 = 0;
  dest[length2] = '\0';
#else
  strncpy(dest, src, size - 1);
  dest[size - 1] = '\0';
#endif
}

bool
EWDevice::DeclareInner(const struct Declaration &declaration,
                       OperationEnvironment &env)
{
  char sTmp[72];

  ewDecelTpIndex = 0;

  if (!TryConnect(env))
    return false;

  // send SetPilotInfo
  WriteWithChecksum(port, "#SPI");
  env.Sleep(50);

  char sPilot[13], sGliderType[9], sGliderID[9];
  convert_string(sPilot, sizeof(sPilot), declaration.pilot_name);
  convert_string(sGliderType, sizeof(sGliderType), declaration.aircraft_type);
  convert_string(sGliderID, sizeof(sGliderID), declaration.aircraft_registration);

  // build string (field 4-5 are GPS info, no idea what to write)
  sprintf(sTmp, "%-12s%-8s%-8s%-12s%-12s%-6s\r", sPilot, sGliderType, sGliderID,
          "" /* GPS Model */, "" /* GPS Serial No. */, "" /* Flight Date */
          /* format unknown, left blank (GPS has a RTC) */);
  port.Write(sTmp);

  if (!port.ExpectString("OK\r", env))
    return false;

  /*
  sprintf(sTmp, "#SUI%02d", 0);           // send pilot name
  WriteWithChecksum(port, sTmp);
  env.Sleep(50);
  port.Write(PilotsName);
  port.Write('\r');

  if (!port.ExpectString("OK\r"))
    return false;

  sprintf(sTmp, "#SUI%02d", 1);           // send type of aircraft
  WriteWithChecksum(port, sTmp);
  env.Sleep(50);
  port.Write(Class);
  port.Write('\r');

  if (!port.ExpectString("OK\r"))
    nDeclErrorCode = 1;

  sprintf(sTmp, "#SUI%02d", 2);           // send aircraft ID
  WriteWithChecksum(port, sTmp);
  env.Sleep(50);
  port.Write(ID);
  port.Write('\r');

  if (!port.ExpectString("OK\r"))
    return false;
  */

  // clear all 6 TP's
  for (int i = 0; i < 6; i++) {
    sprintf(sTmp, "#CTP%02d", i);
    WriteWithChecksum(port, sTmp);
    if (!port.ExpectString("OK\r", env))
      return false;
  }

  for (unsigned j = 0; j < declaration.Size(); ++j)
    if (!AddWaypoint(declaration.GetWaypoint(j), env))
      return false;

  return true;
}

bool
EWDevice::Declare(const struct Declaration &declaration,
                  gcc_unused const Waypoint *home,
                  OperationEnvironment &env)
{
  port.StopRxThread();

  // change to IO Mode baudrate
  unsigned old_baud_rate = port.GetBaudrate();
  if (old_baud_rate == 9600)
    old_baud_rate = 0;
  else if (old_baud_rate != 0 && !port.SetBaudrate(9600))
    return false;

  bool success = DeclareInner(declaration, env);

  // switch to NMEA mode
  port.Write("NMEA\r\n");

  // restore baudrate
  if (old_baud_rate != 0)
    port.SetBaudrate(old_baud_rate);

  return success;
}

bool
EWDevice::AddWaypoint(const Waypoint &way_point, OperationEnvironment &env)
{
  char EWRecord[100];
  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  char NoS, EoW;

  // check for max 6 TP's
  if (ewDecelTpIndex > 6)
    return false;

  // copy at most 6 chars
  const WideToUTF8Converter name_utf8(way_point.name.c_str());
  if (!name_utf8.IsValid())
    return false;

  char IDString[12];
  char *end = CopyTruncateString(IDString, 7, name_utf8);

  // fill up with spaces
  std::fill(end, IDString + 6, ' ');

  // prepare lat
  tmp = (double)way_point.location.latitude.Degrees();
  NoS = 'N';
  if (tmp < 0) {
    NoS = 'S';
    tmp = -tmp;
  }
  DegLat = (int)tmp;
  MinLat = (tmp - DegLat) * 60 * 1000;

  // prepare long
  tmp = (double)way_point.location.longitude.Degrees();
  EoW = 'E';
  if (tmp < 0) {
    EoW = 'W';
    tmp = -tmp;
  }
  DegLon = (int)tmp;
  MinLon = (tmp - DegLon) * 60 * 1000;

  //	Calc E/W and N/S flags

  // prepare flags
  const unsigned EoW_Flag = EoW == 'W' ? 0x08 : 0x04;
  const unsigned NoS_Flag = NoS == 'N' ? 0x01 : 0x02;

  //  Do the calculation
  const unsigned EW_Flags = (short)(EoW_Flag | NoS_Flag);

  // setup command string
  sprintf(EWRecord, "#STP%02X%02X%02X%02X%02X%02X%02X%02X%02X%04X%02X%04X",
          ewDecelTpIndex, IDString[0], IDString[1], IDString[2], IDString[3],
          IDString[4], IDString[5], EW_Flags, DegLat, (int)MinLat / 10, DegLon,
          (int)MinLon / 10);
  WriteWithChecksum(port, EWRecord);

  // wait for response
  if (!port.ExpectString("OK\r", env))
    return false;

  // increase TP index
  ewDecelTpIndex++;

  return true;
}

void
EWDevice::LinkTimeout()
{
  port.Write("NMEA\r\n");
}

static Device *
EWCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new EWDevice(com_port);
}

const struct DeviceRegister ew_driver = {
  _T("EW Logger"),
  _T("EW Logger"),
  DeviceRegister::DECLARE,
  EWCreateOnPort,
};
