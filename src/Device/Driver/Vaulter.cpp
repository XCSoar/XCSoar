// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/Vaulter.hpp"
#include "Device/Driver.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"

using std::string_view_literals::operator""sv;

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
  if (Angle heading; line.ReadBearing(heading)) {
    info.attitude.heading_available.Update(info.clock);
    info.attitude.heading = heading;
  }

  // IAS [m/s]
  if (line.ReadChecked(value)) {
    info.ProvideIndicatedAirspeed(value);
  }

  // Load factor [g]
  if (line.ReadChecked(value)) {
    info.acceleration.ProvideGLoad(value);
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
  double norm;
  Angle bearing;

  // wind speed [m/s]
  bool norm_valid = line.ReadChecked(norm);
  // wind dir [degrees]
  bool bearing_valid = line.ReadBearing(bearing);
  if (norm_valid && bearing_valid) {
    SpeedVector wind(bearing, norm);
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
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
VaulterDevice::PutBallast(double fraction, [[maybe_unused]] double overload, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;
  char buffer[30];
  // vaulter defines the wing loading factor as ratio of no-ballast to weight
  fraction = fraction + 1;
  sprintf(buffer,"PITV1,WL=%0.2f", fraction);
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
VaulterDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);

  const auto type = line.ReadView();
  if (type == "$PITV3"sv)
    return ParsePITV3(line, info);
  else if (type == "$PITV4"sv)
    return ParsePITV4(line, info);
  else if (type == "$PITV5"sv)
    return ParsePITV5(line, info);
  else
    return false;
}

static Device *
VaulterCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new VaulterDevice(com_port);
}

const struct DeviceRegister vaulter_driver = {
  "Vaulter",
  "WSI Vaulter",
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_SETTINGS,
  VaulterCreateOnPort,
};
