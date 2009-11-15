#ifndef TASKBESTMC_HPP
#define TASKBESTMC_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "Util/ZeroFinder.hpp"

/**
 *  Class to solve for MacCready value, being the highest MC value to produce a
 *  pure glide solution for the remainder of the task.
 *  
 * \todo
 * - f() fails if Mc too low for wind, need to account for failed solution
 *
 */
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

/** 
 * Search for best MC.  If fails (MC=0 is below final glide), returns
 * default value.
 * 
 * @param mc Default MacCready value (m/s)
 * 
 * @return Best MC value found or default value if no solution
 */
  virtual double search(const double mc);
protected:
  TaskMacCreadyRemaining tm;
  GlideResult res;
  const AIRCRAFT_STATE &aircraft;
};

#endif

