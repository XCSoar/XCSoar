#include "AirspaceIntersectionVisitor.hpp"

AirspaceInterceptSolution 
AirspaceIntersectionVisitor::intercept(const AbstractAirspace& as,
                                       const AIRCRAFT_STATE& state,
                                       const AirspaceAircraftPerformance &perf) const
{
  AirspaceInterceptSolution solution;
  if (!m_intersections.empty()) {

    const GEOPOINT start = (m_intersections.begin()->first);
    const GEOPOINT end = (m_intersections.begin()->second);

    as.intercept(state, perf, solution, start, end);
  }
  return solution;
}
