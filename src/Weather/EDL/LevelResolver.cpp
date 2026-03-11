// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LevelResolver.hpp"
#include "Request.hpp"

#include <cstdlib>

namespace EDL {

ResolvedLevel
ResolveLevel(AtmosphericPressure qnh, bool qnh_available,
             double altitude) noexcept
{
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

  return {
    static_pressure,
    best_isobar,
  };
}

} // namespace EDL
