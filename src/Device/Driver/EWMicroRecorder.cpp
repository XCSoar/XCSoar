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

#include "Device/Driver/EWMicroRecorder.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Declaration.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Units/System.hpp"
#include "Time/TimeoutClock.hpp"
#include "Operation/Operation.hpp"
#include "Util/StaticString.hxx"

#include <assert.h>
#include <stdio.h>

// Additional sentance for EW support

class EWMicroRecorderDevice : public AbstractDevice {
protected:
  Port &port;

public:
  EWMicroRecorderDevice(Port &_port)
    :port(_port) {}

public:
  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  bool Declare(const Declaration &declaration, const Waypoint *home,
               OperationEnvironment &env) override;
};

static bool
ReadAltitude(NMEAInputLine &line, double &value_r)
{
  double value;
  bool available = line.ReadChecked(value);
  char unit = line.ReadFirstChar();
  if (!available)
    return false;

  if (unit == _T('f') || unit == _T('F'))
    value = Units::ToSysUnit(value, Unit::FEET);

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
  line.Read(type, 16);

  if (StringIsEqual(type, "$PGRMZ")) {
    double value;

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
TryConnect(Port &port, char *user_data, size_t max_user_data,
           OperationEnvironment &env)
{
  port.Flush();
  port.Write('\x02');         // send IO Mode command

  unsigned user_size = 0;

  TimeoutClock timeout(8000);

  while (true) {
    const size_t nbytes = port.WaitAndRead(user_data + user_size,
                                           max_user_data - user_size,
                                           env, timeout);
    if (nbytes == 0)
      return false;

    if (user_size == 0) {
      const char *minus = (const char *)memchr(user_data, '-', nbytes);
      if (minus == nullptr)
        continue;

      user_size = user_data + nbytes - minus;
      memmove(user_data, minus, user_size);
    } else
      user_size += nbytes;

    char *end = (char *)memchr(user_data, '\x13', user_size);
    if (end != nullptr) {
      *end = 0;
      port.Write('\x16');
      return true;
    }

    if (user_size >= max_user_data)
      /* response too large */
      return false;
  }

  return false;
}

static bool
TryConnectRetry(Port &port, char *user_data, size_t max_user_data,
                OperationEnvironment &env)
{
  int retries=10;

  while (--retries)
    if (TryConnect(port, user_data, max_user_data, env))
      return true;

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
WriteCleanString(Port &port, const TCHAR *p,
                 OperationEnvironment &env, unsigned timeout_ms)
{
  NarrowString<256> buffer;
  buffer.SetASCII(p);

  CleanString(buffer.buffer());

  return port.FullWriteString(buffer, env, timeout_ms);
}

static bool
WriteLabel(Port &port, const char *name, OperationEnvironment &env)
{
  return port.FullWriteString(name, env, 1000) &&
    port.FullWrite(": ", 2, env, 500);
}

/**
 * Write a name/value pair to the EW microRecorder.
 */
static bool
WritePair(Port &port, const char *name, const TCHAR *value,
          OperationEnvironment &env)
{
  return WriteLabel(port, name, env) &&
    WriteCleanString(port, value, env, 1000) &&
    port.FullWrite("\r\n", 2, env, 500);
}

static bool
WriteGeoPoint(Port &port, const GeoPoint &value, OperationEnvironment &env)
{
  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  TCHAR NoS, EoW;

  // prepare latitude
  tmp = (double)value.latitude.Degrees();
  NoS = _T('N');
  if (tmp < 0)
    {
      NoS = _T('S');
      tmp = -tmp;
    }

  DegLat = (int)tmp;
  MinLat = (tmp - DegLat) * 60 * 1000;

  // prepare long
  tmp = (double)value.longitude.Degrees();
  EoW = _T('E');
  if (tmp < 0)
    {
      EoW = _T('W');
      tmp = -tmp;
    }

  DegLon = (int)tmp;
  MinLon = (tmp - DegLon) * 60 * 1000;

  char buffer[64];
  sprintf(buffer, "%02d%05d%c%03d%05d%c",
          DegLat, (int)MinLat, NoS,
          DegLon, (int)MinLon, EoW);

  return port.FullWriteString(buffer, env, 1000);
}

static bool
EWMicroRecorderWriteWaypoint(Port &port, const char *type,
                             const Waypoint &way_point,
                             OperationEnvironment &env)
{
  return WriteLabel(port, type, env) &&
    WriteGeoPoint(port, way_point.location, env) &&
    port.Write(' ') &&
    WriteCleanString(port, way_point.name.c_str(), env, 1000) &&
    port.FullWrite("\r\n", 2, env, 500);
}

static bool
DeclareInner(Port &port, const Declaration &declaration,
             OperationEnvironment &env)
{
  assert(declaration.Size() >= 2);
  assert(declaration.Size() <= 12);

  char user_data[2500];

  if (!TryConnectRetry(port, user_data, sizeof(user_data), env))
    return false;

  char *p = strstr(user_data, "USER DETAILS");
  if (p != nullptr)
    *p = 0;

  port.Write('\x18');         // start to upload file

  if (!port.FullWriteString(user_data, env, 5000) ||
      !port.FullWriteString("USER DETAILS\r\n--------------\r\n\r\n",
                            env, 1000))
    return false;

  WritePair(port, "Pilot Name", declaration.pilot_name.c_str(), env);
  WritePair(port, "Competition ID", declaration.competition_id.c_str(), env);
  WritePair(port,  "Aircraft Type", declaration.aircraft_type.c_str(), env);
  WritePair(port,  "Aircraft ID",
            declaration.aircraft_registration.c_str(), env);

  if (!port.FullWriteString("\r\nFLIGHT DECLARATION\r\n-------------------\r\n\r\n",
                            env, 1000))
    return false;

  WritePair(port, "Description", _T("XCSoar task declaration"), env);

  for (unsigned i = 0; i < 11; i++) {
    if (env.IsCancelled())
      return false;

    if (i+1>= declaration.Size()) {
      port.FullWriteString("TP LatLon: 0000000N00000000E TURN POINT\r\n",
                           env, 1000);
    } else {
      const Waypoint &wp = declaration.GetWaypoint(i);
      if (i == 0) {
        if (!EWMicroRecorderWriteWaypoint(port, "Take Off LatLong", wp, env) ||
            !EWMicroRecorderWriteWaypoint(port, "Start LatLon", wp, env))
          return false;
      } else if (i + 1 < declaration.Size()) {
        if (!EWMicroRecorderWriteWaypoint(port, "TP LatLon", wp, env))
          return false;
      }
    }
  }

  const Waypoint &wp = declaration.GetLastWaypoint();
  if (!EWMicroRecorderWriteWaypoint(port, "Finish LatLon", wp, env) ||
      !EWMicroRecorderWriteWaypoint(port, "Land LatLon", wp, env) ||
      env.IsCancelled())
      return false;

  port.Write('\x03');         // finish sending user file

  return port.ExpectString("uploaded successfully", env, 5000);
}

bool
EWMicroRecorderDevice::Declare(const Declaration &declaration,
                               const Waypoint *home,
                               OperationEnvironment &env)
{
  // Must have at least two, max 12 waypoints
  if (declaration.Size() < 2 || declaration.Size() > 12)
    return false;

  port.StopRxThread();

  bool success = DeclareInner(port, declaration, env);

  // go back to NMEA mode
  port.FullWrite("!!\r\n", 4, env, 500);

  return success;
}


static Device *
EWMicroRecorderCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new EWMicroRecorderDevice(com_port);
}

const struct DeviceRegister ew_microrecorder_driver = {
  _T("EW MicroRecorder"),
  _T("EW microRecorder"),
  DeviceRegister::DECLARE,
  EWMicroRecorderCreateOnPort,
};
