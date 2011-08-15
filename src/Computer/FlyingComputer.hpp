/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Math/fixed.hpp"

class GlidePolar;
struct NMEAInfo;
struct DerivedInfo;
struct AircraftState;
struct FlyingState;

/**
 * Detect takeoff and landing.
 *
 * Dependencies: #FlyingComputer.
 */
class FlyingComputer {
  unsigned short time_on_ground;
  unsigned short time_in_flight;

public:
  void Reset();

  void Compute(const GlidePolar &glide_polar,
               const NMEAInfo &basic, const NMEAInfo &last_basic,
               const DerivedInfo &calculated,
               FlyingState &flying);

  void Compute(const GlidePolar &glide_polar,
               const AircraftState &state,
               FlyingState &flying);

protected:
  void Check(FlyingState &state, fixed time);

  /**
   * Update flying state when moving 
   *
   * @param time Time the aircraft is moving
   */
  void Moving(FlyingState &state, fixed time);

  /**
   * Update flying state when stationary 
   *
   * @param time Time the aircraft is stationary
   * @param on_ground Whether the aircraft is known to be on the ground
   */
  void Stationary(FlyingState &state, fixed time);
};

#endif
