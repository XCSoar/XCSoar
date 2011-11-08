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

#ifndef	XCSOAR_TEAM_CODE_CALCULATION_HPP
#define	XCSOAR_TEAM_CODE_CALCULATION_HPP

#include "Util/StaticString.hpp"
#include "Util/TypeTraits.hpp"
#include "Math/fixed.hpp"
#include "Compiler.h"

struct GeoPoint;
class Angle;

class TeamCode
{
  /** The team code */
  TCHAR code[10];

public:
  void Clear() {
    code[0] = 0;
  }

  /**
   * Returns the current team code
   * @return Current team code
   */
  const TCHAR *GetCode() const {
    return code;
  }

  /**
   * Returns the position of the team member in respect to
   * the given reference waypoint
   * @param ref Reference waypoint
   * @return The team member's position
   */
  gcc_pure
  GeoPoint GetLocation(const GeoPoint ref) const;

  /**
   * Returns the bearing from the reference point to the team member
   * @return Bearing from the reference point to the team member
   */
  Angle GetBearing() const;

  /**
   * Returns the distance from the reference point to the team member
   * @return Distance from the reference point to the team member
   */
  fixed GetRange() const;

  /**
   * Updates the team code with the given parameters
   * @param bearing New bearing
   * @param range New range
   */
  void Update(Angle bearing, fixed range);

  /**
   * Updates the team code to the given code
   * @param _code The new team code
   */
  void Update(const TCHAR* _code);
};

static_assert(is_trivial<TeamCode>::value, "type is not trivial");

#endif
