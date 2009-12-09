#include "AirspaceSoonestSort.hpp"

fixed 
AirspaceSoonestSort::metric(const AbstractAirspace &a) const
{
  const GEOPOINT closest = a.closest_point(m_state.Location);

  fixed time_to_intercept = m_max_time;
  fixed intercept_height;

  if (a.intercept_vertical(m_state,
                           closest,
                           m_perf,
                           time_to_intercept,
                           intercept_height)) {
    return time_to_intercept;
  } else {
    return -fixed_one;
  }
}

const AbstractAirspace* 
AirspaceSoonestSort::find_nearest(const Airspaces &airspaces)
{
  const fixed range = m_perf.max_speed()*m_max_time;
  return AirspaceNearestSort::find_nearest(airspaces, range);
}

