#include "AirspaceCircle.hpp"
#include "Navigation/GeoVector.hpp"

AirspaceCircle::AirspaceCircle(const GEOPOINT &loc, 
                               const double _radius):
  center(loc), 
  radius(_radius)
{

}

const FlatBoundingBox 
AirspaceCircle::get_bounding_box(const TaskProjection& task_projection) const
{
  const double eradius = radius*1.42;
  const GEOPOINT ll = GeoVector(eradius,225).end_point(center);
  const GEOPOINT lr = GeoVector(eradius,135).end_point(center);
  const GEOPOINT ur = GeoVector(eradius,45).end_point(center);
  const GEOPOINT ul = GeoVector(eradius,315).end_point(center);

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
  return (loc.Location.distance(center)<=radius);
}

bool 
AirspaceCircle::intersects(const GEOPOINT& g1, const GeoVector &vec) const
{
  // TODO: for testing only
  return true;
}
