/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Device/Driver/EWMicroRecorder.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Units/System.hpp"
#include "PeriodClock.hpp"
#include "Operation/Operation.hpp"
#include "Util/StringUtil.hpp"

#include <tchar.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#ifdef _UNICODE
#include <windows.h>
#endif

// Additional sentance for EW support

class EWMicroRecorderDevice : public AbstractDevice {
protected:
  Port &port;

public:
  EWMicroRecorderDevice(Port &_port)
    :port(_port) {}

public:
  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info);
  virtual bool Declare(const Declaration &declaration, const Waypoint *home,
                       OperationEnvironment &env);
};

static bool
ReadAltitude(NMEAInputLine &line, fixed &value_r)
{
  fixed value;
  bool available = line.read_checked(value);
  char unit = line.read_first_char();
  if (!available)
    return false;

  if (unit == _T('f') || unit == _T('F'))
    value = Units::ToSysUnit(value, unFeet);

  value_r = value;
  return true;
}

bool
EWMicroRecorderDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (StringIsEqual(type, "$PGRMZ")) {
    fixed value;

    /* The normal Garmin $PGRMZ line contains the "true" barometric
       altitude above MSL (corrected with QNH), but EWMicroRecorder
       differs here slightly: it emits the uncorrected barometric
       altitude.  That is the only reason why we catch this sentence
       in the driver instead of letting the generic class NMEAParser
       do it. */
    if (ReadAltitude(line, value))
      info.ProvidePressureAltitude(value);

    return true;
  } else
    return false;
}

static bool
TryConnect(Port &port, char *user_data)
{
  int retries=10;

  while (--retries){

    port.Write('\x02');         // send IO Mode command

    unsigned user_size = 0;
    bool started = false;

    PeriodClock clock;
    clock.update();

    int i;
    while ((i = port.GetChar()) != EOF && !clock.check(8000)) {
      char ch = (char)i;

      if (!started && ch == '-')
        started = true;

      if (started) {
        if (ch == 0x13) {
          port.Write('\x16');
          user_data[user_size] = 0;
          // found end of file
          return true;
        } else {
          if (user_size < sizeof(user_data) - 1) {
            user_data[user_size] = ch;
            user_size++;
          }
        }
      }
    }

  }

  return false;
}

/**
 * "It is important that only alpha numeric characters are included in
 * the declaration, as other characters such as a comma will prevent
 * the resultant .IGC file from being validated."
 *
 * @see http://www.ewavionics.com/products/microRecorder/microRecorder-instructionsfull.pdf
 */
static bool
IsValidEWChar(char ch)
{
  return ch == '\r' || ch == '\n' ||
    ch == ' ' || ch == '-' ||
    (ch >= 'a' && ch <= 'z') ||
    (ch >= 'A' && ch <= 'Z') ||
    (ch >= '0' && ch <= '9');
}

/**
 * Replace all invalid characters according to IsValidEWChar() with a
 * space.
 */
static void
CleanString(char *p)
{
  for (; *p != 0; ++p)
    if (!IsValidEWChar(*p))
      *p = ' ';
}

/**
 * Clean a string and write it to the Port.
 */
static bool
WriteCleanString(Port &port, const TCHAR *p, unsigned timeout_ms)
{
  char buffer[256];

#ifdef _UNICODE
  if (::WideCharToMultiByte(CP_ACP, 0, p, -1, buffer, sizeof(buffer),
                            NULL, NULL) <= 0)
    return false;
#else
  CopyString(buffer, p, sizeof(buffer));
#endif

  CleanString(buffer);

  return port.FullWriteString(buffer, timeout_ms);
}

static bool
EWMicroRecorderPrintf(Port &port, const TCHAR *fmt, ...)
{
  TCHAR EWStr[128];
  va_list ap;

  va_start(ap, fmt);
  _vstprintf(EWStr, fmt, ap);
  va_end(ap);

#ifdef _UNICODE
  char buffer[256];
  if (::WideCharToMultiByte(CP_ACP, 0, EWStr, -1, buffer, sizeof(buffer),
                            NULL, NULL) <= 0)
    return false;
#else
  char *buffer = EWStr;
#endif

  char *p = strchr(buffer, ':');
  if (p != NULL)
    ++p;
  else
    p = buffer;

  CleanString(p);

  return port.FullWriteString(buffer, 1000);
}

/**
 * Write a name/value pair to the EW microRecorder.
 */
static bool
WritePair(Port &port, const char *name, const TCHAR *value)
{
  return port.FullWriteString(name, 1000) &&
    port.FullWrite(": ", 2, 500) &&
    WriteCleanString(port, value, 1000) &&
    port.FullWrite("\r\n", 2, 500);
}

static bool
EWMicroRecorderWriteWaypoint(Port &port,
                             const Waypoint &way_point, const TCHAR *EWType)
{
  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  TCHAR NoS, EoW;

  // prepare latitude
  tmp = (double)way_point.location.latitude.Degrees();
  NoS = _T('N');
  if (tmp < 0)
    {
      NoS = _T('S');
      tmp = -tmp;
    }

  DegLat = (int)tmp;
  MinLat = (tmp - DegLat) * 60 * 1000;

  // prepare long
  tmp = (double)way_point.location.longitude.Degrees();
  EoW = _T('E');
  if (tmp < 0)
    {
      EoW = _T('W');
      tmp = -tmp;
    }

  DegLon = (int)tmp;
  MinLon = (tmp - DegLon) * 60 * 1000;

  return EWMicroRecorderPrintf(port,
                               _T("%s: %02d%05d%c%03d%05d%c %s\r\n"),
                               EWType,
                               DegLat, (int)MinLat, NoS,
                               DegLon, (int)MinLon, EoW,
                               way_point.name.c_str());
}

static bool
DeclareInner(Port &port, const Declaration &declaration,
             OperationEnvironment &env)
{
  assert(declaration.Size() >= 2);
  assert(declaration.Size() <= 12);

  char user_data[2500];

  if (!TryConnect(port, user_data) || env.IsCancelled())
    return false;

  char *p = strstr(user_data, "USER DETAILS");
  if (p != NULL)
    *p = 0;

  port.Write('\x18');         // start to upload file

  if (!port.FullWriteString(user_data, 5000) ||
      !port.FullWriteString("USER DETAILS\r\n--------------\r\n\r\n", 1000))
    return false;

  WritePair(port, "Pilot Name", declaration.pilot_name.c_str());
  WritePair(port, "Competition ID", declaration.competition_id.c_str());
  WritePair(port,  "Aircraft Type", declaration.aircraft_type.c_str());
  WritePair(port,  "Aircraft ID",
            declaration.aircraft_registration.c_str());

  if (!port.FullWriteString("\r\nFLIGHT DECLARATION\r\n-------------------\r\n\r\n",
                            1000))
    return false;

  WritePair(port, "Description", _T("XCSoar task declaration"));

  for (unsigned i = 0; i < 11; i++) {
    if (env.IsCancelled())
      return false;

    if (i+1>= declaration.Size()) {
      port.FullWriteString("TP LatLon: 0000000N00000000E TURN POINT\r\n",
                           1000);
    } else {
      const Waypoint &wp = declaration.GetWaypoint(i);
      if (i == 0) {
        EWMicroRecorderWriteWaypoint(port, wp, _T("Take Off LatLong"));
        EWMicroRecorderWriteWaypoint(port, wp, _T("Start LatLon"));
      } else if (i + 1 < declaration.Size()) {
        EWMicroRecorderWriteWaypoint(port, wp, _T("TP LatLon"));
      }
    }
  }

  const Waypoint &wp = declaration.GetLastWaypoint();
  EWMicroRecorderWriteWaypoint(port, wp, _T("Finish LatLon"));
  EWMicroRecorderWriteWaypoint(port, wp, _T("Land LatLon"));

  if (env.IsCancelled())
      return false;

  port.Write('\x03');         // finish sending user file

  return port.ExpectString("uploaded successfully");
}

bool
EWMicroRecorderDevice::Declare(const Declaration &declaration,
                               const Waypoint *home,
                               OperationEnvironment &env)
{
  // Must have at least two, max 12 waypoints
  if (declaration.Size() < 2 || declaration.Size() > 12)
    return false;

  /* during tests, the EW has taken up to one second to respond to
     the command \x18 */
  port.SetRxTimeout(2500);

  bool success = DeclareInner(port, declaration, env);

  // go back to NMEA mode
  port.FullWrite("!!\r\n", 4, 500);

  return success;
}


static Device *
EWMicroRecorderCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new EWMicroRecorderDevice(com_port);
}

const struct DeviceRegister ewMicroRecorderDevice = {
  _T("EW MicroRecorder"),
  _T("EW microRecorder"),
  DeviceRegister::DECLARE,
  EWMicroRecorderCreateOnPort,
};
