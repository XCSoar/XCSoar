#ifndef TASKBESTMC_HPP
#define TASKBESTMC_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "Util/ZeroFinder.hpp"

class TaskBestMc: 
  public ZeroFinder
{
public:
  TaskBestMc(const std::vector<OrderedTaskPoint*>& tps,
             const unsigned activeTaskPoint,
             const AIRCRAFT_STATE &_aircraft,
             const GlidePolar &_gp,
             const double _mc_min=0.0);
  TaskBestMc(TaskPoint* tp,
             const AIRCRAFT_STATE &_aircraft,
             const GlidePolar &_gp);
  virtual ~TaskBestMc() {};

  virtual double f(const double mc);
  virtual bool valid(const double mc);
  virtual double search(const double mc);
protected:
  TaskMacCreadyRemaining tm;
  GlideResult res;
  const AIRCRAFT_STATE &aircraft;
};

#endif

