#ifndef TASKMINTARGET_HPP
#define TASKMINTARGET_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "GlideSolvers/ZeroFinder.hpp"

class TaskMinTarget: 
  public ZeroFinder
{
public:
  TaskMinTarget(const std::vector<OrderedTaskPoint*>& tps,
                const unsigned activeTaskPoint,
                const AIRCRAFT_STATE &_aircraft,
                const double _t_remaining);
  virtual double f(const double mc);
  virtual bool valid(const double mc);
  virtual double search(const double mc);
protected:
  TaskMacCreadyRemaining tm;
  GLIDE_RESULT res;
  const AIRCRAFT_STATE &aircraft;
  const double t_remaining;
};

#endif

