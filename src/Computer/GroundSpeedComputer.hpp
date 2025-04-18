// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/DeltaTime.hpp"
#include "NMEA/Validity.hpp"
#include "Geo/GeoPoint.hpp"

struct NMEAInfo;

/**
 * A class that computes the ground speed from consecutive location
 * updates if no "real" ground speed input is available.
 */
class GroundSpeedComputer {
  DeltaTime delta_time;

  Validity last_location_available;
  GeoPoint last_location;

public:
  GroundSpeedComputer() {
    delta_time.Reset();
    last_location_available.Clear();
  }

  /**
   * Fill the missing attributes with a fallback.
   */
  void Compute(NMEAInfo &basic);
};
