#ifndef AIRSPACEPOLYGON_HPP
#define AIRSPACEPOLYGON_HPP

#include "AbstractAirspace.hpp"
#include "BaseTask/SearchPoint.hpp"
#include <vector>

class AirspacePolygon: public AbstractAirspace 
{
public:

  AirspacePolygon(const TaskProjection& task_projection);

  const FlatBoundingBox get_bounding_box(const TaskProjection& task_projection) const;

  bool inside(const AIRCRAFT_STATE &loc) const;
  bool intersects(const GEOPOINT& g1, const GeoVector &vec) const;

private:
  std::vector<SearchPoint> border;
public:
#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const AirspacePolygon& as);
#endif
  DEFINE_VISITABLE()
};

#endif
