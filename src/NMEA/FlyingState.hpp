/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_FLYING_STATE_HPP
#define XCSOAR_FLYING_STATE_HPP

#include "Geo/GeoPoint.hpp"

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
  double flight_time;

  /**
   * Time of takeoff.  Negative if aircraft was never observed flying.
   */
  double takeoff_time;

  /**
   * The location of the aircraft when it took off.  If this attribute
   * is "invalid" according to GeoPoint::IsValid(), then the aircraft
   * was never observed flying.
   */
  GeoPoint takeoff_location;

  /**
   * The time stamp when the aircraft released from towing.  This is
   * an estimate based on sink.  If the aircraft was never seen on
   * ground (i.e. XCSoar was switched on while flying), this value is
   * not too useful.  This is negative if the aircraft is assumed to
   * be still towing.
   */
  double release_time;

  /**
   * The time stamp of the last detected 'power-on' - e.g. the last start
   * of the aircraft's engine.
   */
  double power_on_time;
  double power_off_time;

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
  double landing_time;

  GeoPoint landing_location;

  /** Reset flying state as if never flown */
  void Reset();

  bool IsTowing() const {
    return flying && release_time < 0;
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

#endif
