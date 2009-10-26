#ifndef AIRSPACECIRCLE_HPP
#define AIRSPACECIRCLE_HPP

#include "AbstractAirspace.hpp"

class AirspaceCircle: public AbstractAirspace 
{
public:
  AirspaceCircle(const GEOPOINT &loc, const double _radius);

  const FlatBoundingBox get_bounding_box(const TaskProjection& task_projection) const;

  bool inside(const AIRCRAFT_STATE &loc) const;

  virtual void print(std::ostream &f, const TaskProjection &task_projection) const;

private:
  const GEOPOINT center;
  const double radius;
};


#endif
