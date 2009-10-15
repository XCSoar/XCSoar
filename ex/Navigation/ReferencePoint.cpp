#include "ReferencePoint.hpp"
#include "Math/Earth.hpp"

double
ReferencePoint::bearing(const GEOPOINT & ref) const 
{
  return::Bearing(Location, ref);
}

double 
ReferencePoint::distance(const GEOPOINT & ref) const 
{
  return::Distance(Location, ref);
}

double 
ReferencePoint::bearing(const ReferencePoint & ref) const 
{
  return::Bearing(Location, ref.getLocation());
}


double 
ReferencePoint::distance(const ReferencePoint & ref) const
{
  return::Distance(Location, ref.getLocation());
}


