#ifndef TASKGLIDEREQUIRED_HPP
#define TASKGLIDEREQUIRED_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "GlideSolvers/ZeroFinder.hpp"

class TaskGlideRequired: 
  public ZeroFinder
{
public:
  TaskGlideRequired(const std::vector<OrderedTaskPoint*>& tps,
                    const unsigned activeTaskPoint,
                    const AIRCRAFT_STATE &_aircraft,
                    const GlidePolar &gp);
  TaskGlideRequired(TaskPoint* tp,
                    const AIRCRAFT_STATE &_aircraft,
                    const GlidePolar &gp);
  virtual double f(const double mc);
  virtual bool valid(const double mc);
  virtual double search(const double mc);
protected:
  TaskMacCreadyRemaining tm;
  GLIDE_RESULT res;
  const AIRCRAFT_STATE &aircraft;
};

#endif

