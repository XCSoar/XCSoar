// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/AltairPro.hpp"
#include "Device/Driver.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Declaration.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/System.hpp"
#include "Waypoint/Waypoint.hpp"
#include "util/ConvertString.hpp"
#include "util/TruncateString.hpp"
#include "util/Macros.hpp"
#include "time/TimeoutClock.hpp"

#include <stdio.h>
#include <string.h>
#include <cassert>
#include <tchar.h>

using std::string_view_literals::operator""sv;

static constexpr unsigned DECELWPNAMESIZE = 24;                // max size of taskpoint name
static constexpr unsigned DECELWPSIZE = DECELWPNAMESIZE + 25;  // max size of WP declaration

class AltairProDevice : public AbstractDevice {
private:
  Port &port;

  void PutTurnPoint(const char *name, const Waypoint *waypoint,
                    OperationEnvironment &env);
  bool PropertySetGet(const char *name, const char *value,
                      std::span<char> dest,
                      OperationEnvironment &env);

public:
  AltairProDevice(Port &_port):port(_port){}

public:
  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  bool Declare(const struct Declaration &declaration,
               const Waypoint *home,
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

static bool
PTFRS(NMEAInputLine &line, NMEAInfo &info)
{
  // $PTFRS,1,0,0,0,0,0,0,0,5,1,10,0,3,1338313437,0,0,0,,,2*4E
  //
  // $PTFRS,<sealed>,<downloadmode>,<event>,<neartp>,<sealing>,<baromode>,
  //        <decllock>,<newrecavail>,<enl>,<rpm>,<interval>,<error>,<timbase>,
  //        <time>,<secpower>,<secpowerint>,<usup>,<ulit>,
  //        <chargerstate>,<antstate>*CS<CR><LF>

  line.Skip(8);

  unsigned enl;
  if (line.ReadChecked(enl)) {
    info.engine_noise_level = enl;
    info.engine_noise_level_available.Update(info.clock);
  }

  line.Skip(7);

  unsigned supply_voltage;
  if (line.ReadChecked(supply_voltage) && supply_voltage != 0) {
    info.voltage = supply_voltage / 1000.;
    info.voltage_available.Update(info.clock);
  }

  return true;
}

bool
AltairProDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);
  const auto type = line.ReadView();

  // no propriatary sentence

  if (type == "$PGRMZ"sv) {
    double value;
    if (ReadAltitude(line, value))
      info.ProvidePressureAltitude(value);

    return true;
  } else if (type == "$PTFRS"sv) {
    return PTFRS(line, info);
  }

  return false;
}

bool
AltairProDevice::Declare(const struct Declaration &declaration,
                         [[maybe_unused]] const Waypoint *home,
                         OperationEnvironment &env)
{
  port.StopRxThread();

  char Buffer[256];

  if (!PropertySetGet("Pilot", declaration.pilot_name.c_str(),
                      std::span{Buffer}, env))
    return false;

  if (!PropertySetGet("GliderID", declaration.aircraft_registration.c_str(),
                      std::span{Buffer}, env))
    return false;

  if (!PropertySetGet("GliderType", declaration.aircraft_type.c_str(),
                      std::span{Buffer}, env))
    return false;

  /* TODO currently not supported by XCSOAR
   * Pilot2
   * CompetitionID
   * CompetitionClass
   * ObserverID
   * DeclDescription
   * DeclFlightDate
   */

  if (declaration.Size() > 1) {
    PutTurnPoint("DeclTakeoff", nullptr, env);
    PutTurnPoint("DeclLanding", nullptr, env);

    PutTurnPoint("DeclStart", &declaration.GetFirstWaypoint(), env);
    PutTurnPoint("DeclFinish", &declaration.GetLastWaypoint(), env);

    for (unsigned int index=1; index <= 10; index++){
      char TurnPointPropertyName[32];
      StringFormatUnsafe(TurnPointPropertyName, "DeclTurnPoint%d", index);

      if (index < declaration.Size() - 1) {
        PutTurnPoint(TurnPointPropertyName, &declaration.GetWaypoint(index),
                     env);
      } else {
        PutTurnPoint(TurnPointPropertyName, nullptr, env);
      }
    }
  }

  if (!PropertySetGet("DeclAction", "DECLARE",
                      std::span{Buffer}, env))
    return false;

  if (StringIsEqual(&Buffer[9], "LOCKED"))
    // FAILED! try to declare a task on a airborn recorder
    return false;

  // Buffer holds the declaration ticket.
  // but no one is intresting in that
  // eg "2010-11-21 13:01:43 (1)"

  return true;
}



bool
AltairProDevice::PropertySetGet(const char *name, const char *value,
                                std::span<char> dest,
                                OperationEnvironment &env)
{
  port.Flush();

  TimeoutClock timeout(std::chrono::seconds(5));

  // eg $PDVSC,S,FOO,BAR*<cr>\r\n
  char buffer[1024];
  StringFormat(buffer, std::size(buffer),
               "PDVSC,S,%s,%s", name, value);
  PortWriteNMEA(port, buffer, env);

  // expect eg $PDVSC,A,FOO,
  port.ExpectString("PDVSC,A,", env, timeout.GetRemainingOrZero());
  port.ExpectString(name, env, timeout.GetRemainingOrZero());
  port.ExpectString(",", env, timeout.GetRemainingOrZero());

  // read value eg bar
  do {
    const size_t nbytes = port.WaitAndRead(std::as_writable_bytes(dest), env, timeout);

    char *asterisk = (char *)memchr(dest.data(), '*', nbytes);
    if (asterisk != nullptr) {
      *asterisk = 0;
      return true;
    }

    dest = dest.subspan(nbytes);
  } while (!dest.empty());

  return false;
}

void
AltairProDevice::PutTurnPoint(const char *propertyName,
                              const Waypoint *waypoint,
                              OperationEnvironment &env)
{
  char Name[DECELWPNAMESIZE];
  char Buffer[DECELWPSIZE*2];

  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;

  if (waypoint != nullptr){
    if (WideToACPConverter wp_name{waypoint->name.c_str()}; wp_name.IsValid())
      CopyTruncateString(Name, ARRAY_SIZE(Name), wp_name);
    else
      throw std::runtime_error("Invalid string");

    double tmp = (double)waypoint->location.latitude.Degrees();

    if(tmp < 0){
      NoS = 'S';
      tmp *= -1;
    } else NoS = 'N';

    DegLat = (int)tmp;
    MinLat = tmp - DegLat;
    MinLat *= 60;
    MinLat *= 1000;

    tmp = (double)waypoint->location.longitude.Degrees();

    if (tmp < 0){
      EoW = 'W';
      tmp *= -1;
    } else EoW = 'E';

    DegLon = (int)tmp;
    MinLon = tmp  - DegLon;
    MinLon *= 60;
    MinLon *= 1000;

  } else {

    Name[0] = '\0';
    DegLat = 0;
    MinLat = 0;
    DegLon = 0;
    MinLon = 0;
    NoS = 'N';
    EoW = 'E';
  }

  StringFormatUnsafe(Buffer, "%02d%05.0f%c%03d%05.0f%c%s",
                     DegLat, MinLat, NoS, DegLon, MinLon, EoW, Name);

  PropertySetGet(propertyName, Buffer,
                 std::span{Buffer}, env);

}

static Device *
AltairProCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new AltairProDevice(com_port);
}

const struct DeviceRegister altair_pro_driver = {
  _T("Altair RU"),
  _T("Altair Recording Unit"),
  DeviceRegister::DECLARE,
  AltairProCreateOnPort,
};
