#include "AirspacePolygon.hpp"
#include "Math/Earth.hpp"
#include "Navigation/ConvexHull/PolygonInterior.hpp"
#include "Navigation/ConvexHull/GrahamScan.hpp"

AirspacePolygon::AirspacePolygon(const std::vector<GEOPOINT>& pts)
{
  TaskProjection task_projection;
  for (std::vector<GEOPOINT>::const_iterator v = pts.begin();
       v != pts.end(); v++) {
    border.push_back(SearchPoint(*v,task_projection));
  }

  /// \todo remove this pruning
  GrahamScan gs(border);
  border = gs.prune_interior();
}


const GEOPOINT 
AirspacePolygon::get_center()
{
  if (!border.empty()) {
    return border[0].get_location();
  } else {
    return GEOPOINT(0,0);
  }
}


const FlatBoundingBox 
AirspacePolygon::get_bounding_box(const TaskProjection& task_projection)
{
  FLAT_GEOPOINT min;
  FLAT_GEOPOINT max;
  bool empty=true;

  project(task_projection);

  for (std::vector<SearchPoint>::const_iterator v = border.begin();
       v != border.end(); v++) {
    FLAT_GEOPOINT f = v->get_flatLocation();
    if (empty) {
      empty = false;
      min = f; 
      max = f; 
    } else {
      min.Longitude = std::min(min.Longitude, f.Longitude);
      min.Latitude = std::min(min.Latitude, f.Latitude);
      max.Longitude = std::max(max.Longitude, f.Longitude);
      max.Latitude = std::max(max.Latitude, f.Latitude);
    }
  }
  if (!empty) {
    // note +/- 1 to ensure rounding keeps bb valid 
    min.Longitude-= 1; min.Latitude-= 1;
    max.Longitude+= 1; max.Latitude+= 1;
    return FlatBoundingBox(min,max);
  } else {
    return FlatBoundingBox(FLAT_GEOPOINT(0,0),FLAT_GEOPOINT(0,0));
  }
}

bool 
AirspacePolygon::inside(const AIRCRAFT_STATE &loc) const
{
  return PolygonInterior(loc.Location, border);
}


bool 
AirspacePolygon::intersects(const GEOPOINT& start, 
                            const GeoVector &vec,
                            const TaskProjection& task_projection) const
{
  if (PolygonInterior(start, border)) {
    // starts inside!
    return true;
  }

  const GEOPOINT end = vec.end_point(start);
  const FlatRay ray(task_projection.project(start),
                    task_projection.project(end));

  for (unsigned i=0; i+1<border.size(); i++) {
    const FlatRay rthis(border[i].get_flatLocation(), 
                        border[i+1].get_flatLocation());
    if (ray.intersects(rthis)) {
      return true;
    }
  }
  return false;
}

void
AirspacePolygon::project(const TaskProjection &task_projection)
{
  for (unsigned i=0; i<border.size(); i++) {
    border[i].project(task_projection);
  }
}
