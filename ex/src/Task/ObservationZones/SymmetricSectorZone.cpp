#include "SymmetricSectorZone.hpp"
#include "Math/Geometry.hpp"

void SymmetricSectorZone::set_legs(const TaskPoint *previous,
                                   const TaskPoint *current,
                                   const TaskPoint *next) 
{
  double biSector;
  if (!next && previous) { 
    // final
    biSector = previous->bearing(current->getLocation());
  } else if (next && previous) {
    // intermediate
    biSector = BiSector(previous->bearing(current->getLocation()),
                        current->bearing(next->getLocation()));
  } else if (next && !previous) {
    // start
    biSector = next->bearing(current->getLocation());
  } else {
    // single point
    biSector = 0;
  }

  setStartRadial(AngleLimit360(biSector-SectorAngle/2));
  setEndRadial(AngleLimit360(biSector+SectorAngle/2));

}



bool
SymmetricSectorZone::equals(const ObservationZonePoint* other) const
{
  if (CylinderZone::equals(other)) {
    if (const SymmetricSectorZone* z = 
        dynamic_cast<const SymmetricSectorZone*>(other)) {
      return (SectorAngle == z->getSectorAngle());
    }
  }
  return false;
}
