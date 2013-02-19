/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Device/Driver/BlueFlyVario.hpp"
#include "Device/Driver.hpp"
#include "Math/KalmanFilter1d.hpp"
#include "NMEA/Info.hpp"

#include <stdlib.h>

class BlueFlyDevice : public AbstractDevice {
public:
  BlueFlyDevice();

  void LinkTimeout() override;

  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info);
  
  bool ParseBAT(const char *content, NMEAInfo &info);
  bool ParsePRS(const char *content, NMEAInfo &info);
private:
  KalmanFilter1d kalman_filter;
};

void
BlueFlyDevice::LinkTimeout()
{
  kalman_filter.Reset();
}

bool
BlueFlyDevice::ParseBAT(const char *content, NMEAInfo &info)
{
  // e.g.
  // BAT 1234

  char *endptr;
  int mV = (int)strtol(content, &endptr, 16);
  if (endptr == content) return true; 

  do {
    // piecewise linear approximation
    if (mV > 3900) {
      info.battery_level = fixed(70 + (mV - 3900)/10);
      break;
    }
    if (mV > 3700) {
      info.battery_level = fixed(4 + (mV - 3700)/3);
      break;
    }
    if (mV > 3600) {
      info.battery_level = fixed(0.04) * (mV - 3600);
      break;
    }
    // considered empty ...
    info.battery_level = fixed(0);
    break;
  }  while (0);

  if (info.battery_level > fixed(100)) info.battery_level = fixed(100);
  info.battery_level_available.Update(info.clock);

  return true;
}

gcc_pure
static inline
fixed ComputeNoncompVario(const fixed pressure, const fixed d_pressure)
{
  static constexpr fixed FACTOR(-2260.389548275485);
  static constexpr fixed EXPONENT(-0.8097374740609689);
  return fixed(FACTOR * pow(pressure, EXPONENT) * d_pressure);
}

bool
BlueFlyDevice::ParsePRS(const char *content, NMEAInfo &info)
{
  // e.g. PRS 17CBA

  char *endptr;
  long value = strtol(content, &endptr, 16);
  if (endptr != content) {
    AtmosphericPressure pressure = AtmosphericPressure::Pascal(fixed(value));

    kalman_filter.Update(pressure.GetHectoPascal(), fixed(0.25), fixed(0.02));

    info.ProvideNoncompVario(ComputeNoncompVario(kalman_filter.GetXAbs(),
                                                 kalman_filter.GetXVel()));
    info.ProvideStaticPressure(AtmosphericPressure::HectoPascal(kalman_filter.GetXAbs()));
  }

  return true;
}

bool
BlueFlyDevice::ParseNMEA(const char *line, NMEAInfo &info)
{
  if (memcmp(line, "PRS ", 4) == 0)
    return ParsePRS(line + 4, info);
  else if (memcmp(line, "BAT ", 4) == 0)
    return ParseBAT(line + 4, info);
  else
    return false;
}

BlueFlyDevice::BlueFlyDevice()
{
  kalman_filter.SetAccelerationVariance(fixed(0.3));
}


static Device *
BlueFlyCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new BlueFlyDevice();
}

const struct DeviceRegister bluefly_driver = {
  _T("BlueFly"),
  _T("BlueFly Vario"),
  0,
  BlueFlyCreateOnPort,
};
