#ifndef TASKMINTARGET_HPP
#define TASKMINTARGET_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "GlideSolvers/ZeroFinder.hpp"
#include "BaseTask/StartPoint.hpp"

class TaskMinTarget: 
  public ZeroFinder
{
public:
  TaskMinTarget(const std::vector<OrderedTaskPoint*>& tps,
                const unsigned activeTaskPoint,
                const AIRCRAFT_STATE &_aircraft,
                const double _t_remaining,
                StartPoint *_ts);

  virtual double f(const double p);
  virtual bool valid(const double p);
  virtual double search(const double p);
protected:
  void set_range(const double p);
  TaskMacCreadyRemaining tm;
  GLIDE_RESULT res;
  const AIRCRAFT_STATE &aircraft;
  const double t_remaining;
  StartPoint *tp_start;
};

#endif

