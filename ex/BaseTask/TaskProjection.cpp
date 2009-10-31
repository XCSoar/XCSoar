#include "TaskProjection.h"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
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
  location_mid.Longitude = (location_max.Longitude+location_min.Longitude)*0.5;
  location_mid.Latitude = (location_max.Latitude+location_min.Latitude)*0.5;
  cos_midloc = fastcosine(location_mid.Latitude)*SCALE;
}


FlatPoint
TaskProjection::fproject(const GEOPOINT& tp) const
{
  FlatPoint fp((tp.Longitude-location_mid.Longitude)*cos_midloc,
               (tp.Latitude-location_mid.Latitude)*SCALE);
  return fp;
}

GEOPOINT 
TaskProjection::funproject(const FlatPoint& fp) const
{
  GEOPOINT tp;
  tp.Longitude = fp.x/cos_midloc+location_mid.Longitude;
  tp.Latitude = fp.y/SCALE+location_mid.Latitude;
  return tp;
}

FLAT_GEOPOINT 
TaskProjection::project(const GEOPOINT& tp) const
{
  FlatPoint f = fproject(tp);
  FLAT_GEOPOINT fp;
  fp.Longitude = (int)(f.x+0.5);
  fp.Latitude = (int)(f.y+0.5);
  return fp;
}

GEOPOINT 
TaskProjection::unproject(const FLAT_GEOPOINT& fp) const
{
  FlatPoint f(fp.Longitude-0.5,
              fp.Latitude-0.5);
  GEOPOINT tp = funproject(f);
  return tp;
}


double
TaskProjection::fproject_range(const GEOPOINT &tp, const double range) const
{
  GEOPOINT fr;
  ::FindLatitudeLongitude(tp,0,range,&fr);
  FlatPoint f = fproject(fr);
  FlatPoint p = fproject(tp);
  return fabs(f.y-p.y);
}

unsigned
TaskProjection::project_range(const GEOPOINT &tp, const double range) const
{
  return (unsigned)(fproject_range(tp,range)+0.5);
}
