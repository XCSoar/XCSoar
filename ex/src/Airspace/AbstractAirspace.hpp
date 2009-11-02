#ifndef ABSTRACTAIRSPACE_HPP
#define ABSTRACTAIRSPACE_HPP

#include "Util/GenericVisitor.hpp"
#include "Navigation/FlatBoundingBox.hpp"
#include "Navigation/Aircraft.hpp"
#include "Navigation/GeoVector.hpp"

class AbstractAirspace:
  public BaseVisitable<>
{
public:
  virtual const FlatBoundingBox 
    get_bounding_box(const TaskProjection& task_projection) const = 0;

  virtual bool inside(const AIRCRAFT_STATE &loc) const = 0;
  virtual bool intersects(const GEOPOINT& g1, const GeoVector &vec) const = 0;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const AbstractAirspace& as);
#endif
public:
  DEFINE_VISITABLE()
};

#endif
