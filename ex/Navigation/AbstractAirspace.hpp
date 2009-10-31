#ifndef ABSTRACTAIRSPACE_HPP
#define ABSTRACTAIRSPACE_HPP

#include "AirspaceVisitor.hpp"
#include "FlatBoundingBox.hpp"
#include "Aircraft.hpp"

class AbstractAirspace:
  public BaseVisitable<>
{
public:
  DEFINE_VISITABLE()

  virtual const FlatBoundingBox 
    get_bounding_box(const TaskProjection& task_projection) const = 0;

  virtual bool inside(const AIRCRAFT_STATE &loc) const = 0;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const AbstractAirspace& as);
#endif
};

#endif
