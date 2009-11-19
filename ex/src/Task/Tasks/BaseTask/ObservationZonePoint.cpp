#include "ObservationZonePoint.hpp"

bool
ObservationZonePoint::equals(const ObservationZonePoint* other) const
{
  return getLocation().equals(other->getLocation());
}
