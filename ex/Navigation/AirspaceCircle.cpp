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

  return FlatBoundingBox(FLAT_GEOPOINT(std::min(fll.Longitude,
                                                ful.Longitude), 
                                       std::min(fll.Latitude,
                                                flr.Latitude)), 
                         FLAT_GEOPOINT(std::max(flr.Longitude,
                                                fur.Longitude), 
                                       std::max(ful.Latitude,
                                                fur.Latitude)));
}

bool 
AirspaceCircle::inside(const AIRCRAFT_STATE &loc) const
{
  return (::Distance(loc.Location,center)<=radius);
}


void 
AirspaceCircle::print(std::ostream &f, const TaskProjection &task_projection) const
{
  f << "# circle\n";
  for (double t=0; t<=360; t+= 30) {
    GEOPOINT l;
    FindLatitudeLongitude(center, t, radius, &l);
    f << l.Longitude << " " << l.Latitude << "\n";
  }
  f << "\n";
}
