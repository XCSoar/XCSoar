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

#include "Device/Driver/Eye.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/System.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Math/Util.hpp"

class EyeDevice : public AbstractDevice {
public:
  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, NMEAInfo &info) override;

  static bool PEYA(NMEAInputLine &line, NMEAInfo &info);
  static bool PEYI(NMEAInputLine &line, NMEAInfo &info);

protected:
  static bool ReadAcceleration(NMEAInputLine &line, AccelerationState &value_r);
  static bool ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r);
};

bool
EyeDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);
  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "$PEYA"))
    return PEYA(line, info);
  else if (StringIsEqual(type, "$PEYI"))
    return PEYI(line, info);
  else
    return false;
}

bool
EyeDevice::PEYA(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  // Static pressure from aircraft pneumatic system [hPa] (i.e. 1015.5)
  if (line.ReadChecked(value))
    info.ProvideStaticPressure(AtmosphericPressure::HectoPascal(value));

  // Total pressure from aircraft pneumatic system [hPA] (i.e. 1015.5)
  if (line.ReadChecked(value))
    info.ProvidePitotPressure(AtmosphericPressure::HectoPascal(value));

  // Pressure altitude [m] (i.e. 10260)
  if (line.ReadChecked(value))
    info.ProvidePressureAltitude(value);

  // Calculated local QNH [mbar] (i.e. 1013.2)
  if (line.ReadChecked(value))
    info.settings.ProvideQNH(AtmosphericPressure::HectoPascal(value), info.clock);

  // Direction from were the wind blows [°] (0 - 359)
  // Wind speed [km/h]
  SpeedVector wind;
  if (ReadSpeedVector(line, wind))
    info.ProvideExternalWind(wind);

  // True air speed [km/h] (i.e. 183)
  if (line.ReadChecked(value))
    info.ProvideTrueAirspeed(Units::ToSysUnit(value, Unit::KILOMETER_PER_HOUR));

  // Vertical speed from anemometry (m/s) (i.e. +05.4)
  if (line.ReadChecked(value))
    info.ProvideNoncompVario(value);

  // Outside Air Temperature (?C) (i.e. +15.2)
  if (line.ReadChecked(value)) {
    info.temperature = Temperature::FromCelsius(value);
    info.temperature_available = true;
  }

  // Relative humidity [%] (i.e. 095)
  if (line.ReadChecked(value)) {
    info.humidity = value;
    info.humidity_available = true;
  }

  // Condensation altitude [m*1'000] (i.e. 1650)
  // Condensation (cloud) temperature [°C] (es. +05.4)
  // Ground potential temperature [°C] (es. +05.4)

  return true;
}

bool
EyeDevice::PEYI(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  // Roll respect to Earth system - Phi [°] (i.e. +110)
  if (line.ReadChecked(value)) {
    info.attitude.bank_angle_available.Update(info.clock);
    info.attitude.bank_angle = Angle::Degrees(value);
  }

  // Pitch angle respect to Earth system - Theta [°] (i.e.+020)
  if (line.ReadChecked(value)) {
    info.attitude.pitch_angle_available.Update(info.clock);
    info.attitude.pitch_angle = Angle::Degrees(value);
  }

  // Angular rate around x body axis - p [°/s] (i.e. +135)
  // Angular rate around y body axis - q [°/s] (i.e. +135)
  // Angular rate around z body axis - r [°/s] (i.e. +135)
  line.Skip(3);

  // Acceleration along x body axis - [m/s^2] (i.e. +11.54)
  // Acceleration along y body axis - [m/s^2] (i.e. +11.54)
  // Acceleration along z body axis - [m/s^2] (i.e. +11.54)
  ReadAcceleration(line, info.acceleration);

  // Value for turn indicator [ ] (i.e. +12)
  line.Skip();

  // Bear to true North [°] (0° – 359°) (i.e. 248)
  if (line.ReadChecked(value)) {
    info.attitude.heading = Angle::Degrees(value);
    info.attitude.heading_available.Update(info.clock);
  }

  // Bear to magnetic North [°] (0° - 359°) (i.e. 248)
  // Local declination [°] (i.e. +02.3)

  return true;
}

bool
EyeDevice::ReadSpeedVector(NMEAInputLine &line, SpeedVector &value_r)
{
  double bearing, norm;

  bool bearing_valid = line.ReadChecked(bearing);
  bool norm_valid = line.ReadChecked(norm);

  if (bearing_valid && norm_valid) {
    value_r.bearing = Angle::Degrees(bearing);
    value_r.norm = Units::ToSysUnit(norm, Unit::KILOMETER_PER_HOUR);
    return true;
  } else
    return false;
}

bool
EyeDevice::ReadAcceleration(NMEAInputLine &line, AccelerationState &value_r)
{
  double x, y, z;

  bool x_valid = line.ReadChecked(x);
  bool y_valid = line.ReadChecked(y);
  bool z_valid = line.ReadChecked(z);

  if (!x_valid || !y_valid || !z_valid)
    return false;

  value_r.ProvideGLoad(SpaceDiagonal(x, y, z), true);
  return true;
}

static Device *
EyeCreateOnPort(gcc_unused const DeviceConfig &config, gcc_unused Port &com_port)
{
  return new EyeDevice();
}

const struct DeviceRegister eye_driver = {
  _T("EYE"),
  _T("EYE sensor-box (experimental)"),
  0,
  EyeCreateOnPort,
};
