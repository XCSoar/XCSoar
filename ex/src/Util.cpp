#include <math.h>
#include "Util.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include <algorithm>

GEOPOINT FindLocation(const GEOPOINT& p1, 
                      double bearing, 
                      double distance) 
{
  GEOPOINT p2;
  ::FindLatitudeLongitude(p1, bearing, distance, &p2);
  return p2;
}
