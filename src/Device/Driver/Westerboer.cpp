// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/Westerboer.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "Units/System.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"

#include <stdio.h>

using std::string_view_literals::operator""sv;

/**
 * Device driver for Westerboer VW1150.
 * @see http://www.westerboer.de/PDF/VW1150/Datensaetze_V1.2.pdf
 */
class WesterboerDevice : public AbstractDevice {
  Port &port;

public:
  WesterboerDevice(Port &_port):port(_port) {}

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  bool PutMacCready(double mac_cready, OperationEnvironment &env) override;
  bool PutBugs(double bugs, OperationEnvironment &env) override;
};

/**
 * $PWES0,DD,VVVV,MMMM,NNNN,BBBB,SSSS,AAAAA,QQQQQ,IIII,TTTT,UUU,CCCC*CS<CR><LF>
 */
static bool
PWES0(NMEAInputLine &line, NMEAInfo &info)
{
  int i;

  line.Skip(); /* device */

  if (line.ReadChecked(i) && i >= -999 && i <= 999)
    info.ProvideTotalEnergyVario(i / 10.);

  line.Skip(); /* average vario */

  if (line.ReadChecked(i) && i >= -999 && i <= 999)
    info.ProvideNettoVario(i / 10.);

  line.Skip(); /* average netto vario */
  line.Skip(); /* speed to fly */

  unsigned altitude;
  if (line.ReadChecked(altitude) && altitude <= 99999)
    info.ProvidePressureAltitude(altitude);

  if (line.ReadChecked(altitude) && altitude <= 99999)
    info.ProvideBaroAltitudeTrue(altitude);

  unsigned ias, tas;
  bool have_ias = line.ReadChecked(ias) && ias <= 9999;
  bool have_tas = line.ReadChecked(tas) && tas <= 9999;
  if (have_ias && have_tas)
    info.ProvideBothAirspeeds(Units::ToSysUnit(ias / 10.,
                                               Unit::KILOMETER_PER_HOUR),
                              Units::ToSysUnit(tas / 10.,
                                               Unit::KILOMETER_PER_HOUR));

  else if (!have_ias && have_tas)
    info.ProvideTrueAirspeed(Units::ToSysUnit(tas / 10.,
                                              Unit::KILOMETER_PER_HOUR));

  unsigned voltage;
  if (line.ReadChecked(voltage) && voltage <= 999) {
    info.voltage = voltage / 10.;
    info.voltage_available.Update(info.clock);
  }

  if (line.ReadChecked(i) && i >= -999 && i <= 999) {
    info.temperature = Temperature::FromCelsius(i / 10.);
    info.temperature_available = true;
  }

  return true;
}

/**
 * $PWES1,DD,MM,S,AAA,F,V,LLL,BB*CS<CR><LF>
 */
static bool
PWES1(NMEAInputLine &line, NMEAInfo &info)
{
  line.Skip(); /* device */

  int i;
  if (line.ReadChecked(i))
    info.settings.ProvideMacCready(i / 10., info.clock);

  info.switch_state.flight_mode = SwitchState::FlightMode::UNKNOWN;
  if (line.ReadChecked(i)) {
    if (i == 0)
      info.switch_state.flight_mode = SwitchState::FlightMode::CIRCLING;
    else if (i == 1)
      info.switch_state.flight_mode = SwitchState::FlightMode::CRUISE;
  }

  line.Skip(3);

  if (line.ReadChecked(i))
    info.settings.ProvideWingLoading(i / 10., info.clock);

  if (line.ReadChecked(i))
    info.settings.ProvideBugs((100 - i) / 100., info.clock);

  return true;
}

bool
WesterboerDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);

  const auto type = line.ReadView();
  if (type == "$PWES0"sv)
    return PWES0(line, info);

  else if (type == "$PWES1"sv)
    return PWES1(line, info);

  else
    return false;
}

bool
WesterboerDevice::PutMacCready(double _mac_cready, OperationEnvironment &env)
{
  /* 0 .. 60 -> 0.0 .. 6.0 m/s */
  unsigned mac_cready = std::min(uround(_mac_cready * 10), 60u);

  char buffer[64];
  sprintf(buffer, "$PWES4,,%02u,,,,,,,", mac_cready);
  AppendNMEAChecksum(buffer);
  strcat(buffer, "\r\n");
  port.FullWrite(buffer, env, std::chrono::milliseconds{100});

  return true;
}

bool
WesterboerDevice::PutBugs(double _bugs, OperationEnvironment &env)
{
  // Dirtyness from 0 until 20 %
  unsigned bugs = 100 - (unsigned)(_bugs * 100);

  char buffer[64];
  sprintf(buffer, "$PWES4,,,,,%02u,,,,", bugs);
  AppendNMEAChecksum(buffer);
  strcat(buffer, "\r\n");
  port.FullWrite(buffer, env, std::chrono::milliseconds{100});

  return true;
}

static Device *
WesterboerCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new WesterboerDevice(com_port);
}

const struct DeviceRegister westerboer_driver = {
  "Westerboer VW1150",
  "Westerboer VW1150",
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  WesterboerCreateOnPort,
};
