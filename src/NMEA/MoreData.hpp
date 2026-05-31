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

  /** Energy height excess to slow to best glide speed */
  double energy_height;

  /** Nav Altitude + Energy height (m) */
  double TE_altitude;

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

  /** Low-pass filtered brutto vario for output consumers (MergeThread) */
  double filtered_brutto_vario;

  Validity filtered_brutto_vario_available;

  /** Low-pass filtered netto vario for output consumers (trail colouring) */
  double filtered_netto_vario;

  Validity filtered_netto_vario_available;

  void Reset() noexcept;

  constexpr bool NavAltitudeAvailable() const noexcept {
    return baro_altitude_available || gps_altitude_available;
  }

  [[gnu::pure]]
  constexpr bool VarioOutputFilterActive() const noexcept {
    return settings.vario_filter_period_available &&
      settings.vario_filter_period > 0;
  }

  /** Brutto vario after the LX output filter; raw brutto otherwise. */
  [[gnu::pure]]
  constexpr double FilteredBruttoVario() const noexcept {
    if (!VarioOutputFilterActive())
      return brutto_vario;

    return filtered_brutto_vario_available
      ? filtered_brutto_vario
      : brutto_vario;
  }

  /** Netto vario after the LX output filter; raw netto otherwise. */
  [[gnu::pure]]
  constexpr double FilteredNettoVario() const noexcept {
    if (!VarioOutputFilterActive())
      return netto_vario;

    return filtered_netto_vario_available
      ? filtered_netto_vario
      : netto_vario;
  }
};

static_assert(std::is_trivial<MoreData>::value, "type is not trivial");
