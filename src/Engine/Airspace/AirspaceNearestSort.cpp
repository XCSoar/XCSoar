#include "AirspaceNearestSort.hpp"
#include "Airspaces.hpp"
#include "AirspaceVisitor.hpp"
#include "AbstractAirspace.hpp"

void 
AirspaceNearestSort::populate_queue(const Airspaces &airspaces,
                                    const fixed range)
{
  AirspacesInterface::AirspaceVector vectors = 
    airspaces.scan_range(m_location,
                         range,
                         m_condition);

  for (AirspacesInterface::AirspaceVector::iterator v=vectors.begin();
       v != vectors.end(); ++v) {
    const AbstractAirspace *as = v->get_airspace();
    if (as != NULL) {
      const AirspaceInterceptSolution ais = solve_intercept(*as);
      const fixed value = metric(ais);
      if (!negative(value)) {
        m_q.push(std::make_pair(m_reverse? -value:value, std::make_pair(ais, *v)));
      }
    }
  }
}


AirspaceInterceptSolution
AirspaceNearestSort::solve_intercept(const AbstractAirspace &a) const
{
  if (a.Inside(m_location)) {
    return AirspaceInterceptSolution::Invalid();
  } else {
    AirspaceInterceptSolution sol;
    sol.location = a.ClosestPoint(m_location);
    sol.distance = sol.location.Distance(m_location);
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
    visitor.Visit(*m_q.top().second.second.get_airspace());
    m_q.pop();
  } 
}
