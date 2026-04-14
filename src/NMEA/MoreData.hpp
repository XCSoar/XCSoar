// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Info.hpp"

#include <type_traits>

/**
 * A wrapper for NMEA_INFO which adds a few attributes that are cheap
 * to calculate.  They are managed by #BasicComputer inside
 * #MergeThread.
 */
struct MoreData : public NMEAInfo {
  /** Altitude used for navigation (GPS or Baro) */
  double nav_altitude;

  /**
   * Kinetic height \f$v^2/(2g)\f$ from current true airspeed [m].  Zero if
   * airspeed is unavailable.  Not geometric altitude.
   */
  double energy_height;

  /**
   * Total mechanical energy as height: #nav_altitude + #energy_height [m].
   * Same physical units as altitude but not the aircraft geometric position.
   */
  double total_energy_height;

  /** GPS-based vario */
  double gps_vario;

  Validity gps_vario_available;

  /**
   * Current vertical speed (total energy).  This is set to
   * TotalEnergyVario if available, and falls back to GPSVario.  It is
   * maintained by DeviceBlackboard::Vario().
   */
  double brutto_vario;

  Validity brutto_vario_available;

  void Reset() noexcept;

  constexpr bool NavAltitudeAvailable() const noexcept {
    return baro_altitude_available || gps_altitude_available;
  }
};

/**
 * Total mechanical energy height for glide/MacCready (see
 * #MoreData::total_energy_height), or zero when navigation altitude is
 * unavailable.
 */
[[gnu::pure]]
inline double
GlideEnergyHeight(const MoreData &basic) noexcept
{
  return basic.NavAltitudeAvailable() ? basic.total_energy_height : 0.;
}

static_assert(std::is_trivial<MoreData>::value, "type is not trivial");
