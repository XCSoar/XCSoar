#include "TaskProjection.h"

#include <algorithm>

void TaskProjection::scan_location(const GEOPOINT &ref) 
{
  location_min.Longitude = std::min(ref.Longitude,
                                    location_min.Longitude);
  location_max.Longitude = std::max(ref.Longitude,
                                    location_max.Longitude);
  location_min.Latitude = std::min(ref.Latitude,
                                   location_min.Latitude);
  location_max.Latitude = std::max(ref.Latitude,
                                   location_max.Latitude);

  location_mid.Longitude = (location_max.Longitude+location_min.Longitude)/2;
  location_mid.Latitude = (location_max.Latitude+location_min.Latitude)/2;
}

FLAT_GEOPOINT 
TaskProjection::project(const GEOPOINT& tp) const
{
  FLAT_GEOPOINT fp;
  fp.Longitude = (tp.Longitude-location_mid.Longitude)*100+0.5;
  fp.Latitude = (tp.Latitude-location_mid.Latitude)*100+0.5;
  return fp;
}

#include <stdio.h>

void TaskProjection::report()
{
  printf("%g %g - %g \n", location_max.Longitude, location_min.Longitude,
    location_mid.Longitude);
  printf("%g %g - %g \n", location_max.Latitude, location_min.Latitude,
    location_mid.Latitude);
}
