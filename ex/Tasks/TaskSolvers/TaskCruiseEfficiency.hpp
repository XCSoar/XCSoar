#ifndef TASKCRUISEEFFICIENCY_HPP
#define TASKCRUISEEFFICIENCY_HPP

#include "GlideSolvers/TaskMacCreadyTravelled.hpp"
#include "GlideSolvers/ZeroFinder.hpp"
#include <math.h>

class TaskCruiseEfficiency: 
  public ZeroFinder
{
public:
  TaskCruiseEfficiency(const std::vector<OrderedTaskPoint*>& tps,
                       const unsigned activeTaskPoint,
                       const AIRCRAFT_STATE &_aircraft,
                       const double mc);
  virtual double f(const double ce);
  virtual bool valid(const double ce);
  virtual double search(const double ce);

protected:
  TaskMacCreadyTravelled tm;
  GLIDE_RESULT res;
  const AIRCRAFT_STATE &aircraft;
  double dt;
};




#endif
