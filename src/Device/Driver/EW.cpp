// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project


// ToDo

// adding baro alt sentance parser to support baro source priority  if (d == pDevPrimaryBaroSource){...}

#include "Device/Driver/EW.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Declaration.hpp"
#include "Device/Error.hpp"
#include "NMEA/Checksum.hpp"
#include "Operation/Operation.hpp"
#include "util/TruncateString.hpp"
#include "util/ConvertString.hpp"
#include "util/ScopeExit.hxx"

#include <algorithm>

#include <tchar.h>
#include <stdio.h>
#include "Waypoint/Waypoint.hpp"

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
WriteWithChecksum(Port &port, const char *String, OperationEnvironment &env)
{
  port.FullWrite(String, env, std::chrono::seconds{1});

  char sTmp[8];
  sprintf(sTmp, "%02X\r\n", ::NMEAChecksum(String));
  port.FullWrite(sTmp, env, std::chrono::seconds{1});
}

bool
EWDevice::TryConnect(OperationEnvironment &env)
{
  int retries = 10;
  while (--retries) {

    // send IO Mode command
    port.FullWrite("##\r\n", env, std::chrono::seconds{1});

    try {
      port.ExpectString("IO Mode.\r", env);
      return true;
    } catch (const DeviceTimeout &) {
    }

    port.FullFlush(env, std::chrono::milliseconds(100),
                   std::chrono::milliseconds(500));
  }

  return false;
}

static void
convert_string(char *dest, size_t size, const char *src)
{
  strncpy(dest, src, size - 1);
  dest[size - 1] = '\0';
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
  WriteWithChecksum(port, "#SPI", env);
  env.Sleep(std::chrono::milliseconds(50));

  char sPilot[13], sGliderType[9], sGliderID[9];
  convert_string(sPilot, sizeof(sPilot), declaration.pilot_name);
  convert_string(sGliderType, sizeof(sGliderType), declaration.aircraft_type);
  convert_string(sGliderID, sizeof(sGliderID), declaration.aircraft_registration);

  // build string (field 4-5 are GPS info, no idea what to write)
  sprintf(sTmp, "%-12s%-8s%-8s%-12s%-12s%-6s\r", sPilot, sGliderType, sGliderID,
          "" /* GPS Model */, "" /* GPS Serial No. */, "" /* Flight Date */
          /* format unknown, left blank (GPS has a RTC) */);
  port.FullWrite(sTmp, env, std::chrono::seconds{1});

  port.ExpectString("OK\r", env);

  /*
  sprintf(sTmp, "#SUI%02d", 0);           // send pilot name
  WriteWithChecksum(port, sTmp, env);
  env.Sleep(50);
  port.FullWrite(PilotsName, env, std::chrono::seconds{1});
  port.Write('\r');

  port.ExpectString("OK\r");

  sprintf(sTmp, "#SUI%02d", 1);           // send type of aircraft
  WriteWithChecksum(port, sTmp, env);
  env.Sleep(50);
  port.FullWrite(Class, env, std::chrono::seconds{1});
  port.Write('\r');

  port.ExpectString("OK\r");

  sprintf(sTmp, "#SUI%02d", 2);           // send aircraft ID
  WriteWithChecksum(port, sTmp, env);
  env.Sleep(50);
  port.FullWrite(ID, env, std::chrono::seconds{1});
  port.Write('\r');

  port.ExpectString("OK\r");
  */

  // clear all 6 TP's
  for (int i = 0; i < 6; i++) {
    sprintf(sTmp, "#CTP%02d", i);
    WriteWithChecksum(port, sTmp, env);
    port.ExpectString("OK\r", env);
  }

  for (unsigned j = 0; j < declaration.Size(); ++j)
    if (!AddWaypoint(declaration.GetWaypoint(j), env))
      return false;

  return true;
}

bool
EWDevice::Declare(const struct Declaration &declaration,
                  [[maybe_unused]] const Waypoint *home,
                  OperationEnvironment &env)
{
  port.StopRxThread();

  // change to IO Mode baudrate
  unsigned old_baud_rate = port.GetBaudrate();
  if (old_baud_rate == 9600)
    old_baud_rate = 0;
  else if (old_baud_rate != 0)
    port.SetBaudrate(9600);

  AtScopeExit(this, old_baud_rate) {
    // restore baudrate
    if (old_baud_rate != 0)
      port.SetBaudrate(old_baud_rate);
  };

  bool success = DeclareInner(declaration, env);

  // switch to NMEA mode
  port.FullWrite("NMEA\r\n", env, std::chrono::seconds{1});

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
  WriteWithChecksum(port, EWRecord, env);

  // wait for response
  port.ExpectString("OK\r", env);

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
EWCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new EWDevice(com_port);
}

const struct DeviceRegister ew_driver = {
  _T("EW Logger"),
  _T("EW Logger"),
  DeviceRegister::DECLARE,
  EWCreateOnPort,
};
