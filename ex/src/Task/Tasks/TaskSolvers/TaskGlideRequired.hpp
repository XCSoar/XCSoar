#ifndef TASKGLIDEREQUIRED_HPP
#define TASKGLIDEREQUIRED_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "Util/ZeroFinder.hpp"

/**
 *  Class to solve for virtual sink rate such that pure glide at
 *  block MacCready speeds with this sink rate would result in
 *  a solution perfectly on final glide.
 *  
 * \todo
 * - f() fails if Mc too low for wind, need to account for failed solution
 *
 */
class TaskGlideRequired: 
  public ZeroFinder
{
public:
/** 
 * Constructor for ordered task points
 * 
 * @param tps Vector of ordered task points comprising the task
 * @param activeTaskPoint Current active task point in sequence
 * @param _aircraft Current aircraft state
 * @param gp Glide polar to copy for calculations
 */
  TaskGlideRequired(const std::vector<OrderedTaskPoint*>& tps,
                    const unsigned activeTaskPoint,
                    const AIRCRAFT_STATE &_aircraft,
                    const GlidePolar &gp);
/** 
 * Constructor for single task points (non-ordered ones)
 * 
 * @param tp Task point comprising the task
 * @param _aircraft Current aircraft state
 * @param gp Glide polar to copy for calculations
 */
  TaskGlideRequired(TaskPoint* tp,
                    const AIRCRAFT_STATE &_aircraft,
                    const GlidePolar &gp);
  virtual ~TaskGlideRequired() {};

  virtual double f(const double mc);

/** 
 * Search for sink rate to produce final glide solution
 * 
 * @param s Default sink rate value (m/s)
 * 
 * @return Solution sink rate (m/s, down positive)
 */
  virtual double search(const double s);
private:
  TaskMacCreadyRemaining tm;
  GlideResult res;
  const AIRCRAFT_STATE &aircraft;
};

#endif

