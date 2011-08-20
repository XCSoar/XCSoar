#include "AirspaceIntersectionVisitor.hpp"
#include "AirspaceInterceptSolution.hpp"

AirspaceInterceptSolution 
AirspaceIntersectionVisitor::intercept(const AbstractAirspace& as,
                                       const AircraftState& state,
                                       const AirspaceAircraftPerformance &perf,
                                       bool all) const
{
  AirspaceInterceptSolution solution;
  if (m_intersections.empty()) {
    return solution;
  }

  for (AirspaceIntersectionVector::const_iterator it = m_intersections.begin();
       it != m_intersections.end(); ++it) {

    as.Intercept(state, perf, solution, it->first, it->second);

    if (!all) {
      return solution;
    }
  }
  return solution;
}
