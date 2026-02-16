// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/Zander.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Units/System.hpp"
#include "util/StringAPI.hxx"

using std::string_view_literals::operator""sv;

class ZanderDevice : public AbstractDevice {
public:
  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};

static bool
PZAN1(NMEAInputLine &line, NMEAInfo &info)
{
  double baro_altitude;
  if (line.ReadChecked(baro_altitude))
    /* the ZS1 documentation does not specify wheter the altitude is
       STD or QNH, but Franz Poeschl confirmed via email that it is
       the QNH altitude */
    info.ProvideBaroAltitudeTrue(baro_altitude);

  return true;
}

static bool
PZAN2(NMEAInputLine &line, NMEAInfo &info)
{
  double vtas, wnet;

  if (line.ReadChecked(vtas))
    info.ProvideTrueAirspeed(Units::ToSysUnit(vtas, Unit::KILOMETER_PER_HOUR));

  if (line.ReadChecked(wnet))
    info.ProvideTotalEnergyVario((wnet - 10000) / 100.);

  return true;
}

static bool
PZAN3(NMEAInputLine &line, NMEAInfo &info)
{
  // old: $PZAN3,+,026,V,321,035,A,321,035,V*cc
  // new: $PZAN3,+,026,A,321,035,V[,A]*cc

  line.Skip(3);

  Angle direction;
  int speed;
  if (!line.ReadBearing(direction) || !line.ReadChecked(speed))
    return false;

  char okay = line.ReadFirstChar();
  if (okay == 'V') {
    okay = line.ReadFirstChar();
    if (okay == 'V')
      return true;

    if (okay != 'A') {
      line.Skip();
      okay = line.ReadFirstChar();
    }
  }

  if (okay == 'A') {
    SpeedVector wind{direction, Units::ToSysUnit(speed, Unit::KILOMETER_PER_HOUR)};
    info.ProvideExternalWind(wind);
  }

  return true;
}

static bool
PZAN4(NMEAInputLine &line, NMEAInfo &info)
{
  // $PZAN4,1.5,+,20,39,45*cc

  double mc;
  if (line.ReadChecked(mc))
    info.settings.ProvideMacCready(mc, info.clock);

  return true;
}

static bool
PZAN5(NMEAInputLine &line, NMEAInfo &info)
{
  // $PZAN5,VA,MUEHL,123.4,KM,T,234*cc

  const auto state = line.ReadView();

  if (state == "SF"sv)
    info.switch_state.flight_mode = SwitchState::FlightMode::CRUISE;
  else if (state == "VA"sv)
    info.switch_state.flight_mode = SwitchState::FlightMode::CIRCLING;
  else
    info.switch_state.flight_mode = SwitchState::FlightMode::UNKNOWN;

  return true;
}

bool
ZanderDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);

  const auto type = line.ReadView();

  if (type == "$PZAN1"sv)
    return PZAN1(line, info);

  else if (type == "$PZAN2"sv)
    return PZAN2(line, info);

  else if (type == "$PZAN3"sv)
    return PZAN3(line, info);

  else if (type == "$PZAN4"sv)
    return PZAN4(line, info);

  else if (type == "$PZAN5"sv)
    return PZAN5(line, info);

  else
    return false;
}

static Device *
ZanderCreateOnPort([[maybe_unused]] const DeviceConfig &config, [[maybe_unused]] Port &com_port)
{
  return new ZanderDevice();
}

const struct DeviceRegister zander_driver = {
  "Zander",
  "Zander / SDI",
  DeviceRegister::RECEIVE_SETTINGS,
  ZanderCreateOnPort,
};
