#include "SearchPoint.hpp"
#include "Math/FastMath.h"
#include "Navigation/TaskProjection.hpp"
#include <math.h>

SearchPoint::SearchPoint(const GEOPOINT &loc, 
                         const TaskProjection& tp,
                         bool _actual):
  ReferencePoint(loc),
  flatLocation(tp.project(loc)),
  actual(_actual)
{      
}


void 
SearchPoint::project(const TaskProjection& tp) 
{
  flatLocation = tp.project(get_location());
}


bool 
SearchPoint::equals(const SearchPoint& sp) const 
{
  if (sp.get_location() == get_location()) {
    return true;
  } else {
    return false;
  }
}

bool 
SearchPoint::sort(const SearchPoint& sp) const 
{
  return get_location().sort(sp.get_location());
}


unsigned
SearchPoint::flat_distance(const SearchPoint& sp) const
{
  return flatLocation.distance_to(sp.flatLocation);
}
