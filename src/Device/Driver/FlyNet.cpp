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

#include "Device/Driver/FlyNet.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "Math/WindowFilter.hpp"

#include <stdlib.h>

class FlyNetDevice : public AbstractDevice {
  WindowFilter<40> vario_filter;

public:
  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;

  bool ParseBAT(const char *content, NMEAInfo &info);
  bool ParsePRS(const char *content, NMEAInfo &info);
};

bool
FlyNetDevice::ParseBAT(const char *content, NMEAInfo &info)
{
  // e.g.
  // _BAT 3 (30%)
  // _BAT A (100%)
  // _BAT * (charging)

  char *endptr;
  long value = strtol(content, &endptr, 16);
  if (endptr != content) {
    info.battery_level = value * 10.;
    info.battery_level_available.Update(info.clock);
  }

  return true;
}

bool
FlyNetDevice::ParsePRS(const char *content, NMEAInfo &info)
{
  // e.g. _PRS 00017CBA

  // The frequency at which the device sends _PRS sentences
  static constexpr double frequency = 1 / 0.048;

  char *endptr;
  long value = strtol(content, &endptr, 16);
  if (endptr != content) {
    auto pressure = AtmosphericPressure::Pascal(value);

    if (info.static_pressure_available) {
      // Calculate non-compensated vario value

      auto last_pressure = info.static_pressure;

      auto alt = AtmosphericPressure::StaticPressureToPressureAltitude(pressure);
      auto last_alt = AtmosphericPressure::StaticPressureToPressureAltitude(last_pressure);

      auto vario = (alt - last_alt) * frequency;
      vario_filter.Update(vario);

      auto vario_filtered = vario_filter.Average();

      info.ProvideNoncompVario(vario_filtered);
    } else {
      // Reset filter when the first new pressure sentence is received
      vario_filter.Reset();
    }

    info.ProvideStaticPressure(pressure);
  }

  return true;
}

bool
FlyNetDevice::ParseNMEA(const char *line, NMEAInfo &info)
{
  if (memcmp(line, "_PRS ", 5) == 0)
    return ParsePRS(line + 5, info);
  else if (memcmp(line, "_BAT ", 5) == 0)
    return ParseBAT(line + 5, info);
  else
    return false;
}

static Device *
FlyNetCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new FlyNetDevice();
}

const struct DeviceRegister flynet_driver = {
  _T("FlyNet"),
  _T("FlyNet Vario"),
  0,
  FlyNetCreateOnPort,
};
