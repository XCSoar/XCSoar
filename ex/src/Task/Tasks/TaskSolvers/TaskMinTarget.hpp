#ifndef TASKMINTARGET_HPP
#define TASKMINTARGET_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "Util/ZeroFinder.hpp"
#include "Task/TaskPoints/StartPoint.hpp"

/**
 * Optimise target ranges (for adjustable tasks) to produce an estimated
 * time remaining with the current glide polar, equal to a target value.
 *
 * Targets are adjusted along line from min to max linearly; only
 * current and later task points are adjusted.
 *
 * \todo
 * - Allow for other schemes or weightings in how much to adjust each
 *   target.
 */
class TaskMinTarget: 
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
 * @param _t_remaining Desired time remaining (s) of task
 * @param _ts StartPoint of task (to initiate scans)
 */
  TaskMinTarget(const std::vector<OrderedTaskPoint*>& tps,
                const unsigned activeTaskPoint,
                const AIRCRAFT_STATE &_aircraft,
                const GlidePolar &_gp,
                const double _t_remaining,
                StartPoint *_ts);
  virtual ~TaskMinTarget() {};

  virtual double f(const double p);

/** 
 * Test validity of a solution given search parameter
 * 
 * @param p Search parameter (target range parameter [0,1])
 * 
 * @return True if solution is valid
 */
  virtual bool valid(const double p);

/** 
 * Search for target range to produce remaining time equal to
 * value specified in constructor.
 *
 * Running this adjusts the target values for AAT task points. 
 * 
 * @param p Default range (0-1)
 * 
 * @return Range value for solution
 */
  virtual double search(const double p);
private:
  void set_range(const double p);
  TaskMacCreadyRemaining tm;
  GlideResult res;
  const AIRCRAFT_STATE &aircraft;
  const double t_remaining;
  StartPoint *tp_start;
  bool force_current;
};

#endif

