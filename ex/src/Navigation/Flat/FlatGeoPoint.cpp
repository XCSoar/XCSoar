#include "FlatGeoPoint.hpp"
#include "Math/FastMath.h"


unsigned 
FLAT_GEOPOINT::distance_to(const FLAT_GEOPOINT &sp) const
{
  const int dx = (Longitude-sp.Longitude);
  const int dy = (Latitude-sp.Latitude);
  return isqrt4(dx*dx+dy*dy);
}

