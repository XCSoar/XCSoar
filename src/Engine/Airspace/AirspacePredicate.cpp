#include "AirspacePredicate.hpp"
#include "Navigation/Aircraft.hpp"
#include "AbstractAirspace.hpp"

AirspacePredicateTrue AirspacePredicate::always_true;

AirspacePredicateAircraftInside::AirspacePredicateAircraftInside(const AIRCRAFT_STATE& state):
  m_state(state) 
{
}

bool 
AirspacePredicateAircraftInside::operator()(const AbstractAirspace& t) const
{
  return t.inside(m_state);
}

bool
AirspacePredicateHeightRange::operator()(const AbstractAirspace& t) const
{
  return (int)t.get_top().Altitude >= h_min &&
    (int)t.get_base().Altitude <= h_max;
}
