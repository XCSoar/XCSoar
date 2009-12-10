#include "AirspaceSoonestSort.hpp"

AirspaceInterceptSolution 
AirspaceSoonestSort::solve_intercept(const AbstractAirspace &a) const
{
  fixed time_to_intercept = m_max_time;
  AirspaceInterceptSolution sol;
  sol.location = a.closest_point(m_state.Location);

  if (a.intercept_vertical(m_state,
                           sol.location,
                           m_perf,
                           time_to_intercept,
                           sol.altitude)) {
    sol.elapsed_time = time_to_intercept;
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

