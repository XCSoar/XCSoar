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

#include "Device/Driver/Vaulter.hpp"
#include "Device/Driver.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"

static bool
ParsePITV3(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  // bank angle [degrees, positive right]
  if (line.ReadChecked(value)) {
    info.attitude.bank_angle_available.Update(info.clock);
    info.attitude.bank_angle = Angle::Degrees(value);
  }

  // pitch angle [degrees, positive up]
  if (line.ReadChecked(value)) {
    info.attitude.pitch_angle_available.Update(info.clock);
    info.attitude.pitch_angle = Angle::Degrees(value);
  }

  // heading [degrees]
  if (line.ReadChecked(value)) {
    info.attitude.heading_available.Update(info.clock);
    info.attitude.heading = Angle::Degrees(value);
  }

  // IAS [m/s]
  if (line.ReadChecked(value)) {
    info.ProvideIndicatedAirspeed(value);
  }

  // Load factor [g]
  if (line.ReadChecked(value)) {
    info.acceleration.ProvideGLoad(value, true);
  }

  return true;
}


static bool
ParsePITV4(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  // TE vario [m/s]
  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(value);

  return true;
}

static bool
ParsePITV5(NMEAInputLine &line, NMEAInfo &info)
{
  double value;
  double norm, bearing;

  // wind speed [m/s]
  bool norm_valid = line.ReadChecked(norm);
  // wind dir [degrees]
  bool bearing_valid = line.ReadChecked(bearing);
  if (norm_valid && bearing_valid) {
    SpeedVector wind(Angle::Degrees(bearing), norm);
    info.ProvideExternalWind(wind);
  }

  if (line.ReadChecked(value)) {
    // sqrt density ratio
  }
  if (line.ReadChecked(value)) {
    // turbulence
  }

  // climb/cruise switch
  switch (line.Read(-1)) {
  case 1:
    info.switch_state.flight_mode = SwitchState::FlightMode::CRUISE;
    break;
  case 0:
    info.switch_state.flight_mode = SwitchState::FlightMode::CIRCLING;
    break;
  }

  // mc value [m/s]
  if (line.ReadChecked(value)) {
    info.settings.ProvideMacCready(value, info.clock);
  }

  return true;
}

///////////

class VaulterDevice : public AbstractDevice {
  Port &port;

public:
  VaulterDevice(Port &_port):port(_port) {}

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;

  bool PutMacCready(double mc, OperationEnvironment &env) override;
  bool PutBallast(double fraction, double overload,
                  OperationEnvironment &env) override;
};

bool
VaulterDevice::PutMacCready(double mc, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;
  char buffer[30];
  sprintf(buffer,"PITV1,MC=%0.2f", mc);
  return PortWriteNMEA(port, buffer, env);
}

bool
VaulterDevice::PutBallast(double fraction, double overload, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;
  char buffer[30];
  sprintf(buffer,"PITV1,WL=%0.2f", overload);
  return PortWriteNMEA(port, buffer, env);
}

bool
VaulterDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);
  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "$PITV3"))
    return ParsePITV3(line, info);
  else if (StringIsEqual(type, "$PITV4"))
    return ParsePITV4(line, info);
  else if (StringIsEqual(type, "$PITV5"))
    return ParsePITV5(line, info);
  else
    return false;
}

static Device *
VaulterCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new VaulterDevice(com_port);
}

const struct DeviceRegister vaulter_driver = {
  _T("Vaulter"),
  _T("WSI Vaulter"),
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  VaulterCreateOnPort,
};
