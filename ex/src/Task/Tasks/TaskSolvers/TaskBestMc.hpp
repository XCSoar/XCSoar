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
/** 
 * Constructor for ordered task points
 * 
 * @param tps Vector of ordered task points comprising the task
 * @param activeTaskPoint Current active task point in sequence
 * @param _aircraft Current aircraft state
 * @param _gp Glide polar to copy for calculations
 * @param _mc_min Minimum legal value of MacCready (m/s) in search
 */
  TaskBestMc(const std::vector<OrderedTaskPoint*>& tps,
             const unsigned activeTaskPoint,
             const AIRCRAFT_STATE &_aircraft,
             const GlidePolar &_gp,
             const double _mc_min=0.0);
/** 
 * Constructor for single task points (non-ordered ones)
 * 
 * @param tp Task point comprising the task
 * @param _aircraft Current aircraft state
 * @param _gp Glide polar to copy for calculations
 */
  TaskBestMc(TaskPoint* tp,
             const AIRCRAFT_STATE &_aircraft,
             const GlidePolar &_gp);
  virtual ~TaskBestMc() {};

  virtual double f(const double mc);

/** 
 * Test validity of a solution given search parameter
 * 
 * @param mc Search parameter (MacCready setting (m/s))
 * 
 * @return True if solution is valid
 */
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
private:
  TaskMacCreadyRemaining tm;
  GlideResult res;
  const AIRCRAFT_STATE &aircraft;
};

#endif

