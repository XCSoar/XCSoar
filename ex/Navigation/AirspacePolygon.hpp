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

  friend std::ostream& operator<< (std::ostream& f, 
                                   const AirspacePolygon& as);

private:
  std::vector<SearchPoint> border;
};


#endif
