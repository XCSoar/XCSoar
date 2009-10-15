#include <math.h>
#include "Util.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"


GEOPOINT FindLocation(const GEOPOINT& p1, 
                      double bearing, 
                      double distance) 
{
  GEOPOINT p2;
  ::FindLatitudeLongitude(p1, bearing, distance, &p2);
  return p2;
}


GEOPOINT InterpolateLocation(const GEOPOINT& p1,
                             const GEOPOINT& p2, 
                             const double t) 
{
  GEOPOINT p = p1;
  p.Longitude += t*(p2.Longitude-p1.Longitude);
  p.Latitude += t*(p2.Latitude-p1.Latitude);
  return p;
}
