#include "ReferencePoint.hpp"

double
ReferencePoint::bearing(const GEOPOINT & ref) const 
{
  return m_location.bearing(ref);
}

double 
ReferencePoint::distance(const GEOPOINT & ref) const 
{
  return m_location.distance(ref);
}

