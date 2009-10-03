#include "SearchPoint.hpp"

#include <stdio.h>

void 
SearchPoint::project(const TaskProjection& tp) 
{
  flatLocation = tp.project(Location);
};


bool 
SearchPoint::equals(const SearchPoint& sp) const 
{
  if ((sp.Location.Longitude == Location.Longitude)
      &&(sp.Location.Latitude == Location.Latitude)) {
    return true;
  } else {
    return false;
  }
};

bool 
SearchPoint::sort(const SearchPoint& sp) const 
{
  if (Location.Longitude<sp.Location.Longitude) {
    return false;
  } else if (Location.Longitude==sp.Location.Longitude) {
    return Location.Latitude>sp.Location.Latitude;
  } else {
    return true;
  }
}
