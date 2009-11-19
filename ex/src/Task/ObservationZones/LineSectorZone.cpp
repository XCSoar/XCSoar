#include "LineSectorZone.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

GEOPOINT LineSectorZone::get_boundary_parametric(double t) 
{ 
  GEOPOINT loc;
  loc = SectorStart.interpolate(SectorEnd,t);
  return loc;
}

double
LineSectorZone::score_adjustment() 
{
  return 0.0;
}

bool
LineSectorZone::equals(const ObservationZonePoint* other) const
{
  if (SymmetricSectorZone::equals(other)) {
    if (dynamic_cast<const LineSectorZone*>(other)) {
      return true;
    }
  }
  return false;
}

