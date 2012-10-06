/* Copyright_License {

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

#ifndef AIRSPACE_ALTITUDE_HPP
#define AIRSPACE_ALTITUDE_HPP

#include "Math/fixed.hpp"

#include <stdint.h>

class AtmosphericPressure;
struct AltitudeState;

/** Structure to hold airspace altitude boundary data */
struct AirspaceAltitude
{
  enum class Type: uint8_t {
    UNDEFINED,
    MSL,
    AGL,
    FL
  };

  /** Altitude AMSL (m) resolved from type */
  fixed altitude;
  /** Flight level (100ft) for FL-referenced boundary */
  fixed flight_level;
  /** Height above terrain (m) for ground-referenced boundary */
  fixed altitude_above_terrain;
  /** Type of airspace boundary */
  Type type;

  /** 
   * Constructor.  Initialises to zero.
   * 
   * @return Initialised blank object
   */
  AirspaceAltitude()
    :altitude(fixed_zero),
     flight_level(fixed_zero),
     altitude_above_terrain(fixed_zero),
     type(Type::UNDEFINED) {}

  /**
   * Get Altitude AMSL (m) resolved from type.
   * For AGL types, this assumes the terrain height
   * is the terrain height at the aircraft.
   */
  fixed GetAltitude(const AltitudeState &state) const;

  /** Is this altitude reference at or above the aircraft state? */
  bool IsAbove(const AltitudeState &state, const fixed margin = fixed_zero) const;

  /** Is this altitude reference at or below the aircraft state? */
  bool IsBelow(const AltitudeState &state, const fixed margin = fixed_zero) const;

  /**
   * Test whether airspace boundary is the terrain
   *
   * @return True if this altitude limit is the terrain
   */
  bool IsTerrain() const {
    return !positive(altitude_above_terrain) && type == Type::AGL;
  }

  /**
   * Set height of terrain for AGL-referenced airspace;
   * this sets Altitude and must be called before AGL-referenced
   * airspace is considered initialised.
   *
   * @param alt Height of terrain at airspace center
   */
  void SetGroundLevel(const fixed alt);

  /**
   * Is it necessary to call SetGroundLevel() for this AirspaceAltitude?
   */
  bool NeedGroundLevel() const {
    return type == Type::AGL;
  }

  /**
   * Set atmospheric pressure (QNH) for flight-level based
   * airspace.  This sets Altitude and must be called before FL-referenced
   * airspace is considered initialised.
   *
   * @param press Atmospheric pressure model (to obtain QNH)
   */
  void SetFlightLevel(const AtmosphericPressure &press);

  static bool SortHighest(const AirspaceAltitude &a, const AirspaceAltitude &b) {
    return a.altitude > b.altitude;
  }
};

#endif
