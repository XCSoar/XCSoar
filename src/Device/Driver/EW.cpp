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


// ToDo

// adding baro alt sentance parser to support baro source priority  if (d == pDevPrimaryBaroSource){...}

#include "Device/Driver/EW.hpp"
#include "Device/Driver.hpp"
#include "Device/Port.hpp"
#include "NMEA/Checksum.hpp"
#include "Operation.hpp"
#include "Util/StringUtil.hpp"

#include <tchar.h>
#include <stdio.h>
#include "Waypoint/Waypoint.hpp"

#ifdef _UNICODE
#include <windows.h>
#endif

#define  USESHORTTPNAME   1       // hack, soulf be configurable

// Additional sentance for EW support

class EWDevice : public AbstractDevice {
protected:
  Port *port;
  unsigned lLastBaudrate;
  int ewDecelTpIndex;

public:
  EWDevice(Port *_port)
    :port(_port),
     lLastBaudrate(0), ewDecelTpIndex(0) {}

protected:
  bool TryConnect();
  bool AddWaypoint(const Waypoint &way_point);
  bool DeclareInner(const struct Declaration &declaration,
                    OperationEnvironment &env);

public:
  virtual void LinkTimeout();
  virtual bool Declare(const struct Declaration &declaration,
                       OperationEnvironment &env);
};

static void
WriteWithChecksum(Port *port, const char *String)
{
  port->Write(String);

  char sTmp[8];
  sprintf(sTmp, "%02X\r\n", ::NMEAChecksum(String));
  port->Write(sTmp);
}

bool
EWDevice::TryConnect()
{
  int retries=10;
  while (--retries){

    port->Write("##\r\n");         // send IO Mode command
    if (port->ExpectString("IO Mode.\r"))
      return true;

    port->ExpectString("$$$"); // empty input buffer
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
                                      NULL, NULL);
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

  if (!TryConnect())
    return false;

  WriteWithChecksum(port, "#SPI"); // send SetPilotInfo
  env.Sleep(50);

  char sPilot[13], sGliderType[9], sGliderID[9];
  convert_string(sPilot, sizeof(sPilot), declaration.PilotName);
  convert_string(sGliderType, sizeof(sGliderType), declaration.AircraftType);
  convert_string(sGliderID, sizeof(sGliderID), declaration.AircraftReg);

  // build string (field 4-5 are GPS info, no idea what to write)
  sprintf(sTmp, "%-12s%-8s%-8s%-12s%-12s%-6s\r",
          sPilot,
          sGliderType,
          sGliderID,
          "", // GPS Model
          "", // GPS Serial No.
          "" // Flight Date,
                                                  // format unknown,
                                                  // left blank (GPS
                                                  // has a RTC)
  );
  port->Write(sTmp);

  if (!port->ExpectString("OK\r"))
    return false;

  /*
  sprintf(sTmp, "#SUI%02d", 0);           // send pilot name
  WriteWithChecksum(port, sTmp);
  env.Sleep(50);
  port->Write(PilotsName);
  port->Write('\r');

  if (!port->ExpectString("OK\r"))
    return false;

  sprintf(sTmp, "#SUI%02d", 1);           // send type of aircraft
  WriteWithChecksum(port, sTmp);
  env.Sleep(50);
  port->Write(Class);
  port->Write('\r');

  if (!port->ExpectString("OK\r"))
    nDeclErrorCode = 1;

  sprintf(sTmp, "#SUI%02d", 2);           // send aircraft ID
  WriteWithChecksum(port, sTmp);
  env.Sleep(50);
  port->Write(ID);
  port->Write('\r');

  if (!port->ExpectString("OK\r"))
    return false;
  */

  for (int i=0; i<6; i++){                        // clear all 6 TP's
    sprintf(sTmp, "#CTP%02d", i);
    WriteWithChecksum(port, sTmp);
    if (!port->ExpectString("OK\r"))
      return false;
  }
  for (unsigned j = 0; j < declaration.size(); ++j)
    if (!AddWaypoint(declaration.get_waypoint(j)))
      return false;

  return true;
}

bool
EWDevice::Declare(const struct Declaration &declaration,
                  OperationEnvironment &env)
{
  port->StopRxThread();

  lLastBaudrate = port->SetBaudrate(9600L);    // change to IO Mode baudrate

  port->SetRxTimeout(500);                     // set RX timeout to 500[ms]

  bool success = DeclareInner(declaration, env);

  port->Write("NMEA\r\n"); // switch to NMEA mode

  port->SetBaudrate(lLastBaudrate);            // restore baudrate

  port->SetRxTimeout(0);                       // clear timeout
  port->StartRxThread();                       // restart RX thread

  return success;
}


bool
EWDevice::AddWaypoint(const Waypoint &way_point)
{
  char EWRecord[100];
  TCHAR IDString[12];
  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  char NoS, EoW;
  short EoW_Flag, NoS_Flag, EW_Flags;

  if (ewDecelTpIndex > 6){                        // check for max 6 TP's
    return false;
  }

  CopyString(IDString, way_point.Name.c_str(), 7); // copy at most 6 chars

  while (_tcslen(IDString) < 6)                   // fill up with spaces
    _tcscat(IDString, _T(" "));

  #if USESHORTTPNAME > 0
    _tcscpy(&IDString[3], _T("   "));           // truncate to short name
  #endif

  // prepare lat
    tmp = way_point.Location.Latitude.value_degrees();
  NoS = 'N';
  if (tmp < 0)
    {
      NoS = 'S';
      tmp = -tmp;
    }
  DegLat = (int)tmp;
  MinLat = (tmp - DegLat) * 60 * 1000;


  // prepare long
  tmp = way_point.Location.Longitude.value_degrees();
  EoW = 'E';
  if (tmp < 0)
    {
      EoW = 'W';
      tmp = -tmp;
    }
  DegLon = (int)tmp;
  MinLon = (tmp - DegLon) * 60 * 1000;

  //	Calc E/W and N/S flags

  //	Clear flags
  EoW_Flag = 0;                                   // prepare flags
  NoS_Flag = 0;
  EW_Flags = 0;

  if (EoW == 'W')
    {
      EoW_Flag = 0x08;
    }
  else
    {
      EoW_Flag = 0x04;
    }
  if (NoS == 'N')
    {
      NoS_Flag = 0x01;
    }
  else
    {
      NoS_Flag = 0x02;
    }
  //  Do the calculation
  EW_Flags = (short)(EoW_Flag | NoS_Flag);

                                                  // setup command string
  sprintf(EWRecord, "#STP%02X%02X%02X%02X%02X%02X%02X%02X%02X%04X%02X%04X",
                      ewDecelTpIndex,
                      IDString[0],
                      IDString[1],
                      IDString[2],
                      IDString[3],
                      IDString[4],
                      IDString[5],
                      EW_Flags,
                      DegLat, (int)MinLat/10,
                      DegLon, (int)MinLon/10);
  WriteWithChecksum(port, EWRecord);

  if (!port->ExpectString("OK\r")) // wait for response
    return false;

  ewDecelTpIndex = ewDecelTpIndex + 1;            // increase TP index

  return true;
}


void
EWDevice::LinkTimeout()
{
  port->Write("NMEA\r\n");
}

static Device *
EWCreateOnPort(Port *com_port)
{
  return new EWDevice(com_port);
}

const struct DeviceRegister ewDevice = {
  _T("EW Logger"),
  _T("EW Logger"),
  DeviceRegister::DECLARE,
  EWCreateOnPort,
};
