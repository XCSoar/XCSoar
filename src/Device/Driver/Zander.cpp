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

#include "Device/Driver/Zander.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Units/System.hpp"
#include "Util/StringAPI.hxx"

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

  int direction, speed;
  if (!line.ReadChecked(direction) || !line.ReadChecked(speed))
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
    SpeedVector wind(Angle::Degrees(direction),
                     Units::ToSysUnit(speed, Unit::KILOMETER_PER_HOUR));
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

  char state[3];
  line.Read(state, 3);

  if (StringIsEqual(state, "SF"))
    info.switch_state.flight_mode = SwitchState::FlightMode::CRUISE;
  else if (StringIsEqual(state, "VA"))
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
  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "$PZAN1"))
    return PZAN1(line, info);

  if (StringIsEqual(type, "$PZAN2"))
    return PZAN2(line, info);

  if (StringIsEqual(type, "$PZAN3"))
    return PZAN3(line, info);

  if (StringIsEqual(type, "$PZAN4"))
    return PZAN4(line, info);

  if (StringIsEqual(type, "$PZAN5"))
    return PZAN5(line, info);

  return false;
}

static Device *
ZanderCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new ZanderDevice();
}

const struct DeviceRegister zander_driver = {
  _T("Zander"),
  _T("Zander / SDI"),
  DeviceRegister::RECEIVE_SETTINGS,
  ZanderCreateOnPort,
};
