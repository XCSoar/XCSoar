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

double 
GEOPOINT::projected_distance(const GEOPOINT &from,
                             const GEOPOINT &to) const
{
  return ::ProjectedDistance(from, to, *this);
}

bool 
GEOPOINT::equals(const GEOPOINT &other) const
{
  return (Longitude == other.Longitude) && (Latitude == other.Latitude);
}

bool 
GEOPOINT::sort(const GEOPOINT &sp) const
{
  if (Longitude<sp.Longitude) {
    return false;
  } else if (Longitude==sp.Longitude) {
    return Latitude>sp.Latitude;
  } else {
    return true;
  }
}

