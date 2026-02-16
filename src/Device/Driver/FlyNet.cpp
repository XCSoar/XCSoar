// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/FlyNet.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "Math/WindowFilter.hpp"
#include "util/StringCompare.hxx"

#include <stdlib.h>

using std::string_view_literals::operator""sv;

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
  if (auto prs = StringAfterPrefix(line, "_PRS "sv))
    return ParsePRS(prs, info);
  else if (auto bat = StringAfterPrefix(line, "_BAT "sv))
    return ParseBAT(bat, info);
  else
    return false;
}

static Device *
FlyNetCreateOnPort([[maybe_unused]] const DeviceConfig &config, [[maybe_unused]] Port &com_port)
{
  return new FlyNetDevice();
}

const struct DeviceRegister flynet_driver = {
  "FlyNet",
  "FlyNet Vario",
  0,
  FlyNetCreateOnPort,
};
