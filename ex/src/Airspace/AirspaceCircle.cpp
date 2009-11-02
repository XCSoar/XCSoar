#include "AirspaceCircle.hpp"
#include "Math/Earth.hpp"

AirspaceCircle::AirspaceCircle(const GEOPOINT &loc, 
                               const double _radius):
  center(loc), 
  radius(_radius)
{

}

const FlatBoundingBox 
AirspaceCircle::get_bounding_box(const TaskProjection& task_projection) const
{
  GEOPOINT ll;
  GEOPOINT lr;
  GEOPOINT ul;
  GEOPOINT ur;

  FindLatitudeLongitude(center, 225, radius*1.42, &ll);
  FindLatitudeLongitude(center, 135, radius*1.42, &lr);
  FindLatitudeLongitude(center, 45, radius*1.42, &ur);
  FindLatitudeLongitude(center, 315, radius*1.42, &ul);

  FLAT_GEOPOINT fll = task_projection.project(ll);
  FLAT_GEOPOINT flr = task_projection.project(lr);
  FLAT_GEOPOINT ful = task_projection.project(ul);
  FLAT_GEOPOINT fur = task_projection.project(ur);

  // note +/- 1 to ensure rounding keeps bb valid 

  return FlatBoundingBox(FLAT_GEOPOINT(std::min(fll.Longitude,
                                                ful.Longitude)-1, 
                                       std::min(fll.Latitude,
                                                flr.Latitude)-1), 
                         FLAT_GEOPOINT(std::max(flr.Longitude,
                                                fur.Longitude)+1, 
                                       std::max(ful.Latitude,
                                                fur.Latitude)+1));
}

bool 
AirspaceCircle::inside(const AIRCRAFT_STATE &loc) const
{
  return (::Distance(loc.Location,center)<=radius);
}

bool 
AirspaceCircle::intersects(const GEOPOINT& g1, const GeoVector &vec) const
{
  // TODO: for testing only
  return true;
}
