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
  TaskMinTarget(const std::vector<OrderedTaskPoint*>& tps,
                const unsigned activeTaskPoint,
                const AIRCRAFT_STATE &_aircraft,
                const GlidePolar &_gp,
                const double _t_remaining,
                StartPoint *_ts);
  virtual ~TaskMinTarget() {};

  virtual double f(const double p);
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
protected:
  void set_range(const double p);
  TaskMacCreadyRemaining tm;
  GlideResult res;
  const AIRCRAFT_STATE &aircraft;
  const double t_remaining;
  StartPoint *tp_start;
};

#endif

