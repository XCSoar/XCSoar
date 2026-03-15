// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Levels.hpp"

#include "Interface.hpp"

#include <algorithm>
#include <cmath>

namespace EDL {

static AtmosphericPressure
GetCurrentStaticPressure() noexcept
{
  const auto &basic = CommonInterface::Basic();
  if (basic.static_pressure_available)
    return basic.static_pressure;

  const double altitude = basic.NavAltitudeAvailable()
    ? std::max(0., basic.nav_altitude)
    : 3000.;

  const auto &settings = CommonInterface::GetComputerSettings();
  const AtmosphericPressure &qnh = settings.pressure;
  if (settings.pressure_available && qnh.IsPlausible())
    return qnh.QNHAltitudeToStaticPressure(altitude);

  return AtmosphericPressure::PressureAltitudeToStaticPressure(altitude);
}

unsigned
ResolveCurrentLevel() noexcept
{
  const AtmosphericPressure static_pressure = GetCurrentStaticPressure();

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

unsigned
ResolveLevelBelow() noexcept
{
  const double pressure = GetCurrentStaticPressure().GetPascal();

  for (unsigned i = 0; i < NUM_ISOBARS; ++i) {
    if (ISOBARS[i] > pressure)
      return ISOBARS[i];
  }

  return ISOBARS[NUM_ISOBARS - 1];
}

} // namespace EDL
