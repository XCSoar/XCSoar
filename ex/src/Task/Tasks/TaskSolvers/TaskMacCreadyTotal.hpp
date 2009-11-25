#ifndef TASKMACCREADYTOTAL_HPP
#define TASKMACCREADYTOTAL_HPP

#include "TaskMacCready.hpp"

/** 
 * Specialisation of TaskMacCready for total task
 */
class TaskMacCreadyTotal: 
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
  TaskMacCreadyTotal(const std::vector<OrderedTaskPoint*> &_tps,
                     const unsigned _activeTaskPoint,
                     const GlidePolar &_gp);

  double effective_distance(const double time_remaining) const;

  double effective_leg_distance(const double time_remaining) const;

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
