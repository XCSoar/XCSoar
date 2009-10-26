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

  virtual void print(std::ostream &f, const TaskProjection &task_projection) const;

private:
  std::vector<SearchPoint> border;
};


#endif
