#include "FAISectorZone.hpp"

bool
FAISectorZone::equals(const ObservationZonePoint* other) const
{
  if (SymmetricSectorZone::equals(other)) {
    if (dynamic_cast<const FAISectorZone*>(other)) {
      return true;
    }
  }
  return false;
}
