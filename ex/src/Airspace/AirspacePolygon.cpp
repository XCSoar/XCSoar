#include "AirspacePolygon.hpp"
#include "Math/Earth.hpp"
#include "Navigation/ConvexHull/PolygonInterior.hpp"
#include "Navigation/ConvexHull/GrahamScan.hpp"

AirspacePolygon::AirspacePolygon(const TaskProjection& task_projection)
{
  // just for testing, create a random polygon from a convex hull around
  // random points

  const unsigned num = rand()%10+5;
  GEOPOINT c;
  c.Longitude = (rand()%1200-600)/1000.0+0.5;
  c.Latitude = (rand()%1200-600)/1000.0+0.5;

  for (unsigned i=0; i<num; i++) {
    GEOPOINT p=c;
    p.Longitude += (rand()%200)/1000.0;
    p.Latitude += (rand()%200)/1000.0;
    border.push_back(SearchPoint(p,task_projection));
  }
  // random shape generator
  GrahamScan gs(border);
  border = gs.prune_interior();
}

const FlatBoundingBox 
AirspacePolygon::get_bounding_box(const TaskProjection& task_projection) const
{
  FLAT_GEOPOINT min;
  FLAT_GEOPOINT max;
  bool empty=true;

  for (std::vector<SearchPoint>::const_iterator v = border.begin();
       v != border.end(); v++) {
    FLAT_GEOPOINT f = task_projection.project(v->getLocation());
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
AirspacePolygon::intersects(const GEOPOINT& g1, const GeoVector &vec) const
{
  // TODO: for testing only
  return true;
}
