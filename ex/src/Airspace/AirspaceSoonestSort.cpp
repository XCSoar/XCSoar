#include "AirspaceSoonestSort.hpp"

AirspaceInterceptSolution 
AirspaceSoonestSort::solve_intercept(const AbstractAirspace &a) const
{
  const GEOPOINT loc = a.closest_point(m_state.Location);

  AirspaceInterceptSolution sol;
  bool valid = a.intercept(m_state, m_perf, sol, loc, loc);

  if (sol.elapsed_time > m_max_time) {
    valid = false;
  }
  if (!valid) {
    sol.elapsed_time = -fixed_one;
  }
  return sol;
}

fixed
AirspaceSoonestSort::metric(const AirspaceInterceptSolution& sol) const
{
  return sol.elapsed_time;
}

const AbstractAirspace* 
AirspaceSoonestSort::find_nearest(const Airspaces &airspaces)
{
  const fixed range = m_perf.max_speed()*m_max_time;
  return AirspaceNearestSort::find_nearest(airspaces, range);
}
