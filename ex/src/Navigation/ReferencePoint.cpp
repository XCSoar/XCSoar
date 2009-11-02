#include "ReferencePoint.hpp"

double
ReferencePoint::bearing(const GEOPOINT & ref) const 
{
  return Location.bearing(ref);
}

double 
ReferencePoint::distance(const GEOPOINT & ref) const 
{
  return Location.distance(ref);
}

double 
ReferencePoint::bearing(const ReferencePoint & ref) const 
{
  return Location.bearing(ref.getLocation());
}


double 
ReferencePoint::distance(const ReferencePoint & ref) const
{
  return Location.distance(ref.getLocation());
}


