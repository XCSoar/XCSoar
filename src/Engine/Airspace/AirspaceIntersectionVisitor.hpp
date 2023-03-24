// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AirspaceVisitor.hpp"
#include "AirspaceIntersectionVector.hpp"

struct AircraftState;
struct AirspaceInterceptSolution;
class AirspaceAircraftPerformance;

/**
 * Generic visitor for objects in the Airspaces container,
 * for intersection queries.  Sets m_point_intersect by caller.
 */
class AirspaceIntersectionVisitor:
  public AirspaceVisitor
{
protected:
  /** Vector of accumulated intersection pairs */
  AirspaceIntersectionVector intersections;

public:
  /**
   * Called by Airspaces prior to visiting the airspace to
   * make available the point to the visitor.
   *
   * @param p Sorted vector of intercepts
   *
   * @return True if more than one intersection pair
   */
  bool SetIntersections(AirspaceIntersectionVector &&p) {
    intersections = std::move(p);
    return !intersections.empty();
  }

protected:
  /**
   * Find intercept solution of intersections
   *
   * @param as Airspace to test
   * @param state Aircraft state
   * @param perf Performance of aircraft for query
   *
   * @return Solution if any
   */
  [[gnu::pure]]
  AirspaceInterceptSolution Intercept(const AbstractAirspace &as,
                                      const AircraftState &state,
                                      const AirspaceAircraftPerformance &perf) const;
};
