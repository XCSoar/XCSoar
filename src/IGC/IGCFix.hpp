// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "time/BrokenTime.hpp"

struct NMEAInfo;

struct IGCFix
{
  BrokenTime time;

  GeoPoint location;

  bool gps_valid;

  int gps_altitude, pressure_altitude;

  /* extensions follow */

  /**
   * Engine noise level [0 to 999].  Negative if undefined.
   */
  int16_t enl;

  /**
   * Forward thrust, e.g. engine rpm [0 to 999].  Negative if
   * undefined.
   */
  int16_t rpm;

  /**
   * Magnetic heading [degrees].  Negative if undefined.
   */
  int16_t hdm;

  /**
   * True heading [degrees].  Negative if undefined.
   */
  int16_t hdt;

  /**
   * Magnetic track [degrees].  Negative if undefined.
   */
  int16_t trm;

  /**
   * True track [degrees].  Negative if undefined.
   */
  int16_t trt;

  /**
   * Ground speed [km/h].  Negative if undefined.
   */
  int16_t gsp;

  /**
   * Indicated airspeed [km/h].  Negative if undefined.
   */
  int16_t ias;

  /**
   * True airspeed [km/h].  Negative if undefined.
   */
  int16_t tas;

  /**
   * Satellites in use.  Negative if undefined.
   */
  int16_t siu;

  constexpr void ClearExtensions() noexcept {
    enl = rpm = -1;
    hdm = hdt = trm = trt = -1;
    gsp = ias = tas = -1;
    siu = -1;
  }

  constexpr void Clear() noexcept {
    time = BrokenTime::Invalid();
    ClearExtensions();
  }

  constexpr bool IsDefined() const noexcept {
    return time.IsPlausible();
  }

  /**
   * Copy data from the #NMEAInfo object into this.
   *
   * @return true if this object is a valid new fix
   */
  bool Apply(const NMEAInfo &basic) noexcept;
};
