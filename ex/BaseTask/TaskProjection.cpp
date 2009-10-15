#include "TaskProjection.h"
#include "Math/FastMath.h"
#include <algorithm>


#define SCALE 1000.0

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
}

void
TaskProjection::update_fast()
{
  location_mid.Longitude = (location_max.Longitude+location_min.Longitude)/2;
  location_mid.Latitude = (location_max.Latitude+location_min.Latitude)/2;
  cos_midloc = fastcosine(location_mid.Latitude)*SCALE;
}


FLAT_GEOPOINT 
TaskProjection::project(const GEOPOINT& tp) const
{
  FLAT_GEOPOINT fp;
  fp.Longitude = (tp.Longitude-location_mid.Longitude)*cos_midloc+0.5;
  fp.Latitude = (tp.Latitude-location_mid.Latitude)*SCALE+0.5;
//  printf("%d %d\n", fp.Longitude, fp.Latitude);
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
