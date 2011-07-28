/* Copyright_License {

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
#ifndef AIRSPACE_INTERSECTION_VISITOR_HPP
#define AIRSPACE_INTERSECTION_VISITOR_HPP

#include "AirspaceVisitor.hpp"
#include "AbstractAirspace.hpp"

struct AircraftState;
class AirspaceAircraftPerformance;

/**
 * Generic visitor for objects in the Airspaces container,
 * for intersection queries.  Sets m_point_intersect by caller.
 */
class AirspaceIntersectionVisitor:
  public AirspaceVisitor
{
protected:
  AirspaceIntersectionVector m_intersections; /**< Vector of accumulated intersection pairs */ 

public:
/** 
 * Called by Airspaces prior to visiting the airspace to
 * make available the point to the visitor.
 * 
 * @param p Sorted vector of intercepts
 *
 * @return True if more than one intersection pair
 */
  bool set_intersections(const AirspaceIntersectionVector& p) {
    m_intersections = p;
    return !m_intersections.empty();
  }
protected:
/** 
 * Find intercept solution of intersections
 * 
 * @param as Airspace to test
 * @param state Aircraft state
 * @param perf Performance of aircraft for query
 * @param all Whether to find solution for first or all intersections
 * 
 * @return Solution if any
 */
  AirspaceInterceptSolution intercept(const AbstractAirspace& as,
                                      const AircraftState& state,
                                      const AirspaceAircraftPerformance &perf,
                                      bool all=true) const;
};

#endif
