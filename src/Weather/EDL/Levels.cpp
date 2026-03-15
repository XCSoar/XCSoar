// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Levels.hpp"

#include "Interface.hpp"

#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace EDL {

static int
GetCurrentAircraftAltitude() noexcept
{
  const auto &basic = CommonInterface::Basic();

  if (basic.baro_altitude_available)
    return std::max(0, int(std::lround(basic.baro_altitude)));

  if (basic.gps_altitude_available)
    return std::max(0, int(std::lround(basic.gps_altitude)));

  return 3000;
}

bool
IsSupportedIsobar(unsigned isobar) noexcept
{
  for (unsigned value : ISOBARS)
    if (value == isobar)
      return true;

  return false;
}

unsigned
ResolveCurrentLevel() noexcept
{
  const auto &settings = CommonInterface::GetComputerSettings();
  const AtmosphericPressure &qnh = settings.pressure;
  const bool qnh_available = settings.pressure_available;
  const double altitude = GetCurrentAircraftAltitude();

  const AtmosphericPressure static_pressure =
    qnh_available && qnh.IsPlausible()
    ? qnh.QNHAltitudeToStaticPressure(altitude)
    : AtmosphericPressure::PressureAltitudeToStaticPressure(altitude);

  unsigned best_isobar = ISOBARS[0];
  double best_delta = std::abs(static_pressure.GetPascal() - best_isobar);

  for (unsigned i = 1; i < NUM_ISOBARS; ++i) {
    const unsigned isobar = ISOBARS[i];
    const double delta = std::abs(static_pressure.GetPascal() - isobar);
    if (delta < best_delta) {
      best_delta = delta;
      best_isobar = isobar;
    }
  }

  return best_isobar;
}

} // namespace EDL
