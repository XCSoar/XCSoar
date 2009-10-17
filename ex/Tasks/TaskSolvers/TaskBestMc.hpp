#ifndef TASKBESTMC_HPP
#define TASKBESTMC_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "GlideSolvers/ZeroFinder.hpp"

class TaskBestMc: 
  public ZeroFinder
{
public:
  TaskBestMc(const std::vector<OrderedTaskPoint*>& tps,
             const unsigned activeTaskPoint,
             const AIRCRAFT_STATE &_aircraft);
  TaskBestMc(TaskPoint* tp,
             const AIRCRAFT_STATE &_aircraft);

  virtual double f(const double mc);
  virtual bool valid(const double mc);
  virtual double search(const double mc);
protected:
  TaskMacCreadyRemaining tm;
  GLIDE_RESULT res;
  const AIRCRAFT_STATE &aircraft;
};

#endif

