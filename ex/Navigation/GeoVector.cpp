#include "GeoVector.hpp"
#include "Math/Earth.hpp"
#include "Math/NavFunctions.hpp"
#include "Math/Geometry.hpp"
#include <algorithm>

unsigned count_distbearing = 0;

GeoVector::GeoVector(const GEOPOINT &source, const GEOPOINT &target,
                     const bool is_average)
{

  count_distbearing++;

  GEOPOINT loc1 = source;
  GEOPOINT loc2 = target;

  loc1.Latitude *= DEG_TO_RAD;
  loc2.Latitude *= DEG_TO_RAD;
  loc1.Longitude *= DEG_TO_RAD;
  loc2.Longitude *= DEG_TO_RAD;

  const double cloc1Latitude = cos(loc1.Latitude);
  const double cloc2Latitude = cos(loc2.Latitude);
  const double dlon = loc2.Longitude-loc1.Longitude;

  const double s1 = sin((loc2.Latitude-loc1.Latitude)/2);
  const double s2 = sin(dlon/2);
  const double a= std::max(0.0,std::min(1.0,s1*s1+cloc1Latitude*cloc2Latitude*s2*s2));
  Distance = 6371000.0*2.0*atan2(sqrt(a),sqrt(1.0-a));

  const double y = sin(dlon)*cloc2Latitude;
  const double x = cloc1Latitude*sin(loc2.Latitude)
    -sin(loc1.Latitude)*cloc2Latitude*cos(dlon);

  Bearing = (x==0 && y==0) ? 0:AngleLimit360(atan2(y,x)*RAD_TO_DEG);

  // TODO: handle is_average
}

bool operator != (const GEOPOINT&g1, const GEOPOINT &g2) {
  return (g1.Latitude != g2.Latitude) || (g1.Longitude != g2.Longitude);
}

bool operator != (const GeoVector&g1, const GeoVector &g2) {
  return (g1.Distance != g2.Distance) || (g1.Bearing != g2.Bearing);
}

GEOPOINT 
GeoVector::end_point(const GEOPOINT &source) const
{
  GEOPOINT p;
  ::FindLatitudeLongitude(source, Bearing, Distance, &p);
  return p;
}

GEOPOINT 
GeoVector::mid_point(const GEOPOINT &source) const
{
  GEOPOINT p;
  ::FindLatitudeLongitude(source, Bearing, Distance/2.0, &p);
  return p;
}


