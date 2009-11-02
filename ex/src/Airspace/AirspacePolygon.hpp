#ifndef AIRSPACEPOLYGON_HPP
#define AIRSPACEPOLYGON_HPP

#include "AbstractAirspace.hpp"
#include "Navigation/SearchPointVector.hpp"
#include <vector>

class AirspacePolygon: public AbstractAirspace 
{
public:

  AirspacePolygon(const TaskProjection& task_projection);

  const FlatBoundingBox get_bounding_box(const TaskProjection& task_projection) const;

  bool inside(const AIRCRAFT_STATE &loc) const;
  bool intersects(const GEOPOINT& g1, const GeoVector &vec,
                  const TaskProjection& task_projection) const;

private:
  SearchPointVector border;
public:
#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const AirspacePolygon& as);
#endif
  DEFINE_VISITABLE()
};

#endif
