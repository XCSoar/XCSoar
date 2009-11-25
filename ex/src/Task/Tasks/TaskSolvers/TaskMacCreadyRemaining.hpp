#ifndef TASKMACCREADYREMAINING_HPP
#define TASKMACCREADYREMAINING_HPP

#include "TaskMacCready.hpp"

/** 
 * Specialisation of TaskMacCready for task remaining
 */
class TaskMacCreadyRemaining: 
  public TaskMacCready
{
public:
/** 
 * Constructor for ordered task points
 * 
 * @param _tps Vector of ordered task points comprising the task
 * @param _activeTaskPoint Current active task point in sequence
 * @param _gp Glide polar to copy for calculations
 */
  TaskMacCreadyRemaining(const std::vector<OrderedTaskPoint*> &_tps,
                         const unsigned _activeTaskPoint,
                         const GlidePolar _gp);

/** 
 * Constructor for single task points (non-ordered ones)
 * 
 * @param tp Task point comprising the task
 * @param gp Glide polar to copy for calculations
 */
  TaskMacCreadyRemaining(TaskPoint* tp,
                         const GlidePolar gp);

/** 
 * Set ranges of all remaining task points
 * 
 * @param tp Range parameter [0,1]
 * @param force_current If true, will force active AAT point (even if inside) to move
 */
  void set_range(const double tp, const bool force_current);

private:

  virtual GlideResult tp_solution(const unsigned i,
                                   const AIRCRAFT_STATE &aircraft, 
                                   double minH) const;
  virtual double get_min_height(const AIRCRAFT_STATE &aircraft) const {
    return 0.0;
  }
  virtual const AIRCRAFT_STATE get_aircraft_start(const AIRCRAFT_STATE &aircraft) const;

};

#endif
