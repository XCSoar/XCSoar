/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "DeltaTime.hpp"

struct NMEAInfo;
struct DerivedInfo;
struct AircraftState;
struct FlyingState;

/**
 * Detect takeoff and landing.
 */
class FlyingComputer {
  DeltaTime delta_time;

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

  fixed stationary_since;
  GeoPoint stationary_at;

  fixed climbing_since;
  fixed climbing_altitude;

  fixed sinking_since;

  GeoPoint sinking_location;

  fixed sinking_altitude;

public:
  void Reset();

  void Compute(fixed takeoff_speed,
               const NMEAInfo &basic,
               const DerivedInfo &calculated,
               FlyingState &flying);

  void Compute(fixed takeoff_speed,
               const AircraftState &state, fixed dt,
               FlyingState &flying);

  /**
   * Finish the landing detection.  If landing has not been detected,
   * but the aircraft has not been moving, this force-detects the
   * landing now.  Call at the end of a replay.
   */
  void Finish(FlyingState &flying, fixed time);

protected:
  void CheckRelease(FlyingState &state, fixed time, const GeoPoint &location,
                    fixed altitude);

  /**
   * Check for monotonic climb.  This check is used for "flying"
   * detection in a wave, when ground speed is low, no airspeed is
   * available and no map was loaded.
   *
   * @return true if the aircraft has been climbing for more than 10
   * seconds
   */
  bool CheckClimbing(fixed time, fixed altitude);

  void Check(FlyingState &state, fixed time);

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
