#include "LineSectorZone.hpp"
#include "Navigation/GeoVector.hpp"

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
