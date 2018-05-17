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

#include "Device/Driver/Westerboer.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "Units/System.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"

#include <tchar.h>
#include <stdio.h>

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
  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "$PWES0"))
    return PWES0(line, info);

  if (StringIsEqual(type, "$PWES1"))
    return PWES1(line, info);

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
  port.Write(buffer);

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
  port.Write(buffer);

  return true;
}

static Device *
WesterboerCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new WesterboerDevice(com_port);
}

const struct DeviceRegister westerboer_driver = {
  _T("Westerboer VW1150"),
  _T("Westerboer VW1150"),
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  WesterboerCreateOnPort,
};
