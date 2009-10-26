#ifndef ABSTRACTAIRSPACE_HPP
#define ABSTRACTAIRSPACE_HPP

#include "FlatBoundingBox.hpp"
#include "Aircraft.hpp"

class AbstractAirspace 
{
public:

  virtual const FlatBoundingBox 
    get_bounding_box(const TaskProjection& task_projection) const = 0;

  virtual bool inside(const AIRCRAFT_STATE &loc) const = 0;

  virtual void print(std::ostream &f, const TaskProjection &task_projection) const {};
};

#endif
