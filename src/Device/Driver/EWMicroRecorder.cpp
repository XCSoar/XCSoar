// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project


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
#include "time/TimeoutClock.hpp"
#include "Operation/Operation.hpp"
#include "util/StaticString.hxx"

#include <cassert>
#include <stdio.h>

using std::string_view_literals::operator""sv;

// Additional sentance for EW support

class EWMicroRecorderDevice : public AbstractDevice {
protected:
  Port &port;

public:
  EWMicroRecorderDevice(Port &_port)
    :port(_port) {}

public:
  /* virtual methods from class Device */
  bool EnableNMEA(OperationEnvironment &env) override;
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  bool Declare(const Declaration &declaration, const Waypoint *home,
               OperationEnvironment &env) override;
};

bool
EWMicroRecorderDevice::EnableNMEA(OperationEnvironment &env)
{
  port.FullWrite("!!\r\n", env, std::chrono::milliseconds(500));
  return true;
}

static bool
ReadAltitude(NMEAInputLine &line, double &value_r)
{
  double value;
  bool available = line.ReadChecked(value);
  char unit = line.ReadFirstChar();
  if (!available)
    return false;

  if (unit == 'f' || unit == 'F')
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

  const auto type = line.ReadView();
  if (type == "$PGRMZ"sv) {
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

  TimeoutClock timeout(std::chrono::seconds(8));

  while (true) {
    const size_t nbytes = port.WaitAndRead(std::as_writable_bytes(std::span{user_data + user_size, max_user_data - user_size}),
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
static void
WriteCleanString(Port &port, const char *p,
                 OperationEnvironment &env,
                 std::chrono::steady_clock::duration timeout)
{
  NarrowString<256> buffer;
  buffer.SetASCII(p);

  CleanString(buffer.buffer());

  port.FullWrite(buffer, env, timeout);
}

static void
WriteLabel(Port &port, const char *name, OperationEnvironment &env)
{
  port.FullWrite(name, env, std::chrono::seconds(1));
  port.FullWrite(": ", env, std::chrono::milliseconds(500));
}

/**
 * Write a name/value pair to the EW microRecorder.
 */
static void
WritePair(Port &port, const char *name, const char *value,
          OperationEnvironment &env)
{
  WriteLabel(port, name, env);
  WriteCleanString(port, value, env, std::chrono::seconds(1));
  port.FullWrite("\r\n", env, std::chrono::milliseconds(500));
}

static void
WriteGeoPoint(Port &port, const GeoPoint &value, OperationEnvironment &env)
{
  int DegLat, DegLon;
  double tmp, MinLat, MinLon;
  char NoS, EoW;

  // prepare latitude
  tmp = (double)value.latitude.Degrees();
  NoS = 'N';
  if (tmp < 0)
    {
      NoS = 'S';
      tmp = -tmp;
    }

  DegLat = (int)tmp;
  MinLat = (tmp - DegLat) * 60 * 1000;

  // prepare long
  tmp = (double)value.longitude.Degrees();
  EoW = 'E';
  if (tmp < 0)
    {
      EoW = 'W';
      tmp = -tmp;
    }

  DegLon = (int)tmp;
  MinLon = (tmp - DegLon) * 60 * 1000;

  char buffer[64];
  sprintf(buffer, "%02d%05d%c%03d%05d%c",
          DegLat, (int)MinLat, NoS,
          DegLon, (int)MinLon, EoW);

  port.FullWrite(buffer, env, std::chrono::seconds(1));
}

static void
EWMicroRecorderWriteWaypoint(Port &port, const char *type,
                             const Waypoint &way_point,
                             OperationEnvironment &env)
{
  WriteLabel(port, type, env);
  WriteGeoPoint(port, way_point.location, env);
  port.Write(' ');
  WriteCleanString(port, way_point.name.c_str(),
                   env, std::chrono::seconds(1));
  port.FullWrite("\r\n", env, std::chrono::milliseconds(500));
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

  port.FullWrite(user_data, env, std::chrono::seconds(5));
  port.FullWrite("USER DETAILS\r\n--------------\r\n\r\n",
                 env, std::chrono::seconds(1));

  WritePair(port, "Pilot Name", declaration.pilot_name.c_str(), env);
  WritePair(port, "Competition ID", declaration.competition_id.c_str(), env);
  WritePair(port,  "Aircraft Type", declaration.aircraft_type.c_str(), env);
  WritePair(port,  "Aircraft ID",
            declaration.aircraft_registration.c_str(), env);

  port.FullWrite("\r\nFLIGHT DECLARATION\r\n-------------------\r\n\r\n",
                 env, std::chrono::seconds(1));

  WritePair(port, "Description", "XCSoar task declaration", env);

  for (unsigned i = 0; i < 11; i++) {
    if (i+1>= declaration.Size()) {
      port.FullWrite("TP LatLon: 0000000N00000000E TURN POINT\r\n",
                     env, std::chrono::seconds(1));
    } else {
      const Waypoint &wp = declaration.GetWaypoint(i);
      if (i == 0) {
        EWMicroRecorderWriteWaypoint(port, "Take Off LatLong", wp, env);
        EWMicroRecorderWriteWaypoint(port, "Start LatLon", wp, env);
      } else if (i + 1 < declaration.Size()) {
        EWMicroRecorderWriteWaypoint(port, "TP LatLon", wp, env);
      }
    }
  }

  const Waypoint &wp = declaration.GetLastWaypoint();
  EWMicroRecorderWriteWaypoint(port, "Finish LatLon", wp, env);
  EWMicroRecorderWriteWaypoint(port, "Land LatLon", wp, env);

  port.Write('\x03');         // finish sending user file

  port.ExpectString("uploaded successfully",
                    env, std::chrono::seconds(5));
  return true;
}

bool
EWMicroRecorderDevice::Declare(const Declaration &declaration,
                               [[maybe_unused]] const Waypoint *home,
                               OperationEnvironment &env)
{
  // Must have at least two, max 12 waypoints
  if (declaration.Size() < 2 || declaration.Size() > 12)
    return false;

  port.StopRxThread();

  return DeclareInner(port, declaration, env);
}


static Device *
EWMicroRecorderCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new EWMicroRecorderDevice(com_port);
}

const struct DeviceRegister ew_microrecorder_driver = {
  "EW MicroRecorder",
  "EW microRecorder",
  DeviceRegister::DECLARE,
  EWMicroRecorderCreateOnPort,
};
