// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "time/Stamp.hpp"

#include <type_traits>

/**
 * Structure for flying state (takeoff/landing)
 */
struct FlyingState
{
  /** True if airborne, False otherwise */
  bool flying;
  /** Detects when glider is on ground for several seconds */
  bool on_ground;
  /** True when in powered flight */
  bool powered;

  /** Time of flight */
  FloatDuration flight_time;

  /**
   * Time of takeoff.  Negative if aircraft was never observed flying.
   */
  TimeStamp takeoff_time;

  /**
   * The location of the aircraft when it took off.  If this attribute
   * is "invalid" according to GeoPoint::IsValid(), then the aircraft
   * was never observed flying.
   */
  GeoPoint takeoff_location;

  /**
   * The altitude of the aircraft when it took off.  Only valid is
   * takeoff_time is not negative.
   */
  double takeoff_altitude;

  /**
   * The time stamp when the aircraft released from towing.  This is
   * an estimate based on sink.  If the aircraft was never seen on
   * ground (i.e. XCSoar was switched on while flying), this value is
   * not too useful.  This is negative if the aircraft is assumed to
   * be still towing.
   */
  TimeStamp release_time;

  /**
   * The time stamp of the last detected 'power-on' - e.g. the last start
   * of the aircraft's engine.
   */
  TimeStamp power_on_time;
  TimeStamp power_off_time;

  /**
   * The location of the aircraft when it released from towing.
   * Always check GeoPoint::IsValid() before using this value.
   */
  GeoPoint release_location;

  /**
   * The location of the aircraft when it powered it's engine on
   * for the last time.
   * Always check GeoPoint::IsValid() before using this value.
   */
  GeoPoint power_on_location;
  GeoPoint power_off_location;

  /**
   * The location that is most far away from the release location.
   * Always check GeoPoint::IsValid() before using this value.
   */
  GeoPoint far_location;

  /**
   * The distance from #release_location to #far_location.  This value
   * is negative if it was not calculated yet.
   */
  double far_distance;

  /**
   * Time stamp of the landing.  Invalid if negative.
   */
  TimeStamp landing_time;

  GeoPoint landing_location;

  /** Reset flying state as if never flown */
  void Reset();

  /**
   * Was a take-off recorded?  This validates the fields
   * #takeoff_time, #takeoff_location and #takeoff_altitude.
   */
  bool HasTakenOff() const noexcept {
    return takeoff_time.IsDefined();
  }

  bool IsTowing() const {
    return flying && !release_time.IsDefined();
  }

  /**
   * Are we currently gliding?  That is, flying without engine and
   * without being towed.
   */
  bool IsGliding() const {
    return flying && !powered && !IsTowing();
  }
};

static_assert(std::is_trivial<FlyingState>::value, "type is not trivial");
