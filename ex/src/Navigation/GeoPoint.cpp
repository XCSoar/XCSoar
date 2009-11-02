#include "Navigation/GeoPoint.hpp"
#include "Math/Earth.hpp"

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

double
GEOPOINT::distance(const GEOPOINT &other) const
{
  return ::Distance(*this, other);
}

double
GEOPOINT::bearing(const GEOPOINT &other) const
{
  return ::Bearing(*this, other);
}
