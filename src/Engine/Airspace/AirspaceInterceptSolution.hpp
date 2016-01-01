/* Copyright_License {

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

#ifndef AIRSPACE_INTERCEPT_SOLUTION_HPP
#define AIRSPACE_INTERCEPT_SOLUTION_HPP

#include "Geo/GeoPoint.hpp"

#include <type_traits>

/**
 *  Structure to hold data for intercepts between aircraft and airspace.
 *  (interior or exterior)
 */
struct AirspaceInterceptSolution
{
  /** Location of intercept point */
  GeoPoint location;
  /** Distance from observer to intercept point (m) */
  double distance;
  /** Altitude AMSL (m) of intercept point */
  double altitude;
  /** Estimated time (s) for observer to reach intercept point */
  double elapsed_time;

  AirspaceInterceptSolution() = default;

private:
  AirspaceInterceptSolution(double _distance, double _elapsed_time)
    :distance(_distance), elapsed_time(_elapsed_time) {}

public:
  static AirspaceInterceptSolution Invalid() {
    return AirspaceInterceptSolution(-1, -1);
  }

  void SetInvalid() {
    distance = -1;
    elapsed_time = -1;
  }

  /**
   * Determine whether this solution is valid
   *
   * @return True if solution is valid
   */
  bool IsValid() const {
    return elapsed_time >= 0;
  }

  bool IsEarlierThan(const AirspaceInterceptSolution &other) const {
    return IsValid() && (!other.IsValid() ||
                         elapsed_time < other.elapsed_time);
  }
};

static_assert(std::is_trivial<AirspaceInterceptSolution>::value,
              "type is not trivial");

#endif
