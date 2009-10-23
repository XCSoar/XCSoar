#ifndef TASKOPTTARGET_HPP
#define TASKOPTTARGET_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "GlideSolvers/ZeroFinder.hpp"
#include "BaseTask/StartPoint.hpp"
#include "BaseTask/AATIsolineSegment.hpp"

class TaskOptTarget: 
  public ZeroFinder
{
public:
  TaskOptTarget(const std::vector<OrderedTaskPoint*>& tps,
                const unsigned activeTaskPoint,
                const AIRCRAFT_STATE &_aircraft,
                AATPoint& _tp_current,
                StartPoint *_ts);
  virtual double f(const double p);
  virtual bool valid(const double p);
  virtual double search(const double p);
protected:
  void set_target(const double p);
  TaskMacCreadyRemaining tm;
  GLIDE_RESULT res;
  const AIRCRAFT_STATE &aircraft;
  StartPoint *tp_start;
  AATPoint &tp_current;
  AATIsolineSegment iso;
};


#endif
