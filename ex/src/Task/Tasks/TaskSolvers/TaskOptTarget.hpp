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
 * - Refactor, since passing in AATPoint is a hack
 */

class TaskOptTarget: 
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
 * @param _tp_current Active AATPoint
 * @param _ts StartPoint of task (to initiate scans)
 */
  TaskOptTarget(const std::vector<OrderedTaskPoint*>& tps,
                const unsigned activeTaskPoint,
                const AIRCRAFT_STATE &_aircraft,
                const GlidePolar &_gp,
                AATPoint& _tp_current,
                StartPoint *_ts);
  virtual ~TaskOptTarget() {};

  virtual double f(const double p);

/** 
 * Test validity of a solution given search parameter
 * 
 * @param p Search parameter (isoline parameter [0,1])
 * 
 * @return True if solution is valid
 */
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

private:
  void set_target(const double p); /**< Sets target location along isoline */
  TaskMacCreadyRemaining tm; /**< Object to calculate remaining task statistics */
  GlideResult res; /**< Glide solution used in search */
  const AIRCRAFT_STATE &aircraft; /**< Observer */
  StartPoint *tp_start; /**< Start of task */
  AATPoint &tp_current; /**< Active AATPoint */
  AATIsolineSegment iso; /**< Isoline for active AATPoint target */
};


#endif
