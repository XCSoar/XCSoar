#ifndef TASKGLIDEREQUIRED_HPP
#define TASKGLIDEREQUIRED_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "Util/ZeroFinder.hpp"

/**
 *  Class to solve for virtual sink rate such that pure glide at
 *  block MacCready speeds with this sink rate would result in
 *  a solution perfectly on final glide.
 *  
 */
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
  virtual ~TaskGlideRequired() {};

  virtual double f(const double mc);
  virtual bool valid(const double mc);

/** 
 * Search for sink rate to produce final glide solution
 * 
 * @param s Default sink rate value (m/s)
 * 
 * @return Solution sink rate (m/s, down positive)
 */
  virtual double search(const double s);
protected:
  TaskMacCreadyRemaining tm;
  GlideResult res;
  const AIRCRAFT_STATE &aircraft;
};

#endif

