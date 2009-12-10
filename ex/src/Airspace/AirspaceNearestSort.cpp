#include "AirspaceNearestSort.hpp"
#include "Airspaces.hpp"
#include "AirspaceVisitor.hpp"

void 
AirspaceNearestSort::populate_queue(const Airspaces &airspaces,
                                    const fixed range)
{
  Airspaces::AirspaceVector vectors = airspaces.scan_range(m_state,
                                                           range,
                                                           m_condition);

  for (Airspaces::AirspaceVector::iterator v=vectors.begin();
       v != vectors.end(); ++v) {
    const AbstractAirspace &as = *v->get_airspace();
    const AirspaceInterceptSolution ais = solve_intercept(as);
    const fixed value = metric(ais);
    if (!negative(value)) {
      m_q.push(std::make_pair(m_reverse? -value:value, std::make_pair(ais, *v)));
    }
  }
}


AirspaceInterceptSolution
AirspaceNearestSort::solve_intercept(const AbstractAirspace &a) const
{
  if (a.inside(m_state.Location)) {
    AirspaceInterceptSolution null_sol;
    return null_sol;
  } else {
    AirspaceInterceptSolution sol;
    sol.location = a.closest_point(m_state.Location);
    sol.distance = sol.location.distance(m_state.Location);
    return sol;
  }
}

fixed 
AirspaceNearestSort::metric(const AirspaceInterceptSolution& sol) const
{
  return sol.distance;
}


const AbstractAirspace*
AirspaceNearestSort::find_nearest(const Airspaces &airspaces,
                                  const fixed range)
{
  populate_queue(airspaces, range);

  if (!m_q.empty()) {
    return m_q.top().second.second.get_airspace();
  } else {
    return NULL;
  }
}


void
AirspaceNearestSort::visit_sorted(const Airspaces &airspaces,
                                  AirspaceVisitor &visitor,
                                  const fixed range) 
{
  populate_queue(airspaces, range);

  while (!m_q.empty()) {
    m_q.top().second.second.get_airspace()->Accept(visitor);
    m_q.pop();
  } 
}
