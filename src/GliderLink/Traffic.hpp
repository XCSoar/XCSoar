// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "GliderLinkId.hpp"
#include "Geo/GeoPoint.hpp"
#include "NMEA/Validity.hpp"
#include "util/StaticString.hxx"
#include "Rough/RoughAltitude.hpp"
#include "Rough/RoughSpeed.hpp"
#include "Rough/RoughAngle.hpp"

#include <type_traits>

struct GliderLinkTraffic {
  /** Was the direction of the target received? */
  bool track_received;

  /** Was the speed of the target received? */
  bool speed_received;

  /** Was the absolute altitude of the target received? */
  bool altitude_received;

  /** Was the climb_rate of the target received? */
  bool climb_rate_received;

  /** Is this object valid, or has it expired already? */
  Validity valid;

  /** Location of the GliderLink target */
  GeoPoint location;

  /** TrackBearing of the GliderLink target */
  RoughAngle track;

  /** Speed of the GliderLink target */
  RoughSpeed speed;

  /** Altitude of the GliderLink target */
  RoughAltitude altitude;

  /** Climb rate of the GliderLink target */
  double climb_rate;

  /** GliderLink id of the GliderLink target */
  GliderLinkId id;

  /** (if exists) Name of the GliderLink target */
  StaticString<10> name;


  bool IsDefined() const {
    return valid;
  }

  /**
   * Does the target have a name?
   * @return True if a name has been assigned to the target
   */
  bool HasName() const {
    return !name.empty();
  }

  void Clear() {
    valid.Clear();
    name.clear();
  }

  /**
   * Clear this object if its data has expired.
   *
   * @return true if the object is still valid
   */
  bool Refresh(TimeStamp time) noexcept {
    valid.Expire(time, std::chrono::minutes(5));
    return valid;
  }
};

static_assert(std::is_trivial<GliderLinkTraffic>::value, "type is not trivial");
