#ifndef TASKOPTTARGET_HPP
#define TASKOPTTARGET_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "Util/ZeroFinder.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/AATIsolineSegment.hpp"

/**
 * Adjust target lateral offset for active task point to minimise
 * elapsed time.
 *
 * \todo
 * - Merge with TaskMinTarget?
 */

class TaskOptTarget: 
  public ZeroFinder
{
public:
  TaskOptTarget(const std::vector<OrderedTaskPoint*>& tps,
                const unsigned activeTaskPoint,
                const AIRCRAFT_STATE &_aircraft,
                const GlidePolar &_gp,
                AATPoint& _tp_current,
                StartPoint *_ts);
  virtual ~TaskOptTarget() {};

  virtual double f(const double p);
  virtual bool valid(const double p);

/** 
 * Search for active task point's target isoline to minimise elapsed time
 * to finish.
 *
 * Running this adjusts the target values for the active task point. 
 * 
 * @param p Default isoline value (0-1)
 * 
 * @return Isoline value for solution
 */
  virtual double search(const double p);

protected:
  void set_target(const double p); /**< Sets target location along isoline */
  TaskMacCreadyRemaining tm; /**< Object to calculate remaining task statistics */
  GlideResult res; /**< Glide solution used in search */
  const AIRCRAFT_STATE &aircraft; /**< Observer */
  StartPoint *tp_start; /**< Start of task */
  AATPoint &tp_current; /**< Active AATPoint */
  AATIsolineSegment iso; /**< Isoline for active AATPoint target */
};


#endif
