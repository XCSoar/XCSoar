#include "ObservationZonePoint.hpp"

bool
ObservationZonePoint::equals(const ObservationZonePoint* other) const
{
  return get_location().equals(other->get_location());
}

