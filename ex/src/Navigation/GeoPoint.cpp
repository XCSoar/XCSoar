#include "Navigation/GeoPoint.hpp"

GEOPOINT 
GEOPOINT::parametric(const GEOPOINT &delta, const double t) const
{
  return (*this)+delta*t;
}

GEOPOINT 
GEOPOINT::interpolate(const GEOPOINT &end, const double t) const
{
  return (*this)+(end-(*this))*t;
}
