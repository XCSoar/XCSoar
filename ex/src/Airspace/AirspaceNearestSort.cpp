#include "AirspaceNearestSort.hpp"
#include "Airspaces.hpp"
#include "AirspaceVisitor.hpp"

const AbstractAirspace*
AirspaceNearestSort::find_nearest(const Airspaces &airspaces,
                                  const fixed range)
{
  Airspaces::AirspaceVector vectors = airspaces.scan_range(m_state,
                                                           range,
                                                           m_condition);

  Queue m_q;

  for (Airspaces::AirspaceVector::iterator v=vectors.begin();
       v != vectors.end(); ++v) {
    const AbstractAirspace &as = *v->get_airspace();
    const fixed value = metric(as);
    if (!negative(value)) {
      m_q.push(std::make_pair(m_reverse? -value:value, *v));
    }
  }
  if (!m_q.empty()) {
    return m_q.top().second.get_airspace();
  } else {
    return NULL;
  }
}

fixed 
AirspaceNearestSort::metric(const AbstractAirspace &a) const
{
  if (a.inside(m_state.Location)) {
    return fixed_zero;
  } else {
    return a.closest_point(m_state.Location).distance(m_state.Location);
  }
}

void
AirspaceNearestSort::visit_sorted(const Airspaces &airspaces,
                                  AirspaceVisitor &visitor,
                                  const fixed range) 
{
  Airspaces::AirspaceVector vectors = airspaces.scan_range(m_state,
                                                           range,
                                                           m_condition);

  Queue m_q;

  for (Airspaces::AirspaceVector::iterator v=vectors.begin();
       v != vectors.end(); ++v) {
    const AbstractAirspace &as = *v->get_airspace();
    const fixed value = metric(as);
    if (!negative(value)) {
      m_q.push(std::make_pair(m_reverse? -value:value, *v));
    }
  }

  while (!m_q.empty()) {
    m_q.top().second.get_airspace()->Accept(visitor);
    m_q.pop();
  } 
}
