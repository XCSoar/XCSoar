/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_FLYING_COMPUTER_HPP
#define XCSOAR_FLYING_COMPUTER_HPP

#include "StateClock.hpp"
#include "Math/fixed.hpp"
#include "Geo/GeoPoint.hpp"

struct NMEAInfo;
struct DerivedInfo;
struct AircraftState;
struct FlyingState;

/**
 * Detect takeoff and landing.
 */
class FlyingComputer {
  /**
   * Tracks the duration the aircraft has been stationary.
   */
  StateClock<60, 5> stationary_clock;

  /**
   * Tracks the duration the aircraft has been moving.
   */
  StateClock<30, 5> moving_clock;

  /**
   * If the aircraft is currenly assumed to be moving, then this
   * denotes the initial moving time stamp.  This gets reset to a
   * negative value when the aircraft is stationary for a certain
   * amount of time.
   */
  fixed moving_since;

  /**
   * If the aircraft is currently assumed to be moving, then this
   * denotes the location when moving started initially.  This
   * attribute is only valid if #moving_since is non-negative.
   */
  GeoPoint moving_at;

  fixed sinking_since;

  GeoPoint sinking_location;

  fixed sinking_altitude;

public:
  void Reset();

  void Compute(fixed takeoff_speed,
               const NMEAInfo &basic, const NMEAInfo &last_basic,
               const DerivedInfo &calculated,
               FlyingState &flying);

  void Compute(fixed takeoff_speed,
               const AircraftState &state, fixed dt,
               FlyingState &flying);

protected:
  void CheckRelease(FlyingState &state, fixed time, const GeoPoint &location,
                    fixed altitude);

  void Check(FlyingState &state, fixed time, const GeoPoint &location);

  /**
   * Update flying state when moving 
   *
   * @param time Time the aircraft is moving
   */
  void Moving(FlyingState &state, fixed time, fixed dt,
              const GeoPoint &location);

  /**
   * Update flying state when stationary 
   *
   * @param time Time the aircraft is stationary
   * @param on_ground Whether the aircraft is known to be on the ground
   */
  void Stationary(FlyingState &state, fixed time, fixed dt,
                  const GeoPoint &location);
};

#endif
