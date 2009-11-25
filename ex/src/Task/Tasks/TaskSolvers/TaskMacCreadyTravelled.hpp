#ifndef TASKMACCREADYTRAVELLED_HPP
#define TASKMACCREADYTRAVELLED_HPP

#include "TaskMacCready.hpp"

/** 
 * Specialisation of TaskMacCready for task travelled
 */
class TaskMacCreadyTravelled: 
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
  TaskMacCreadyTravelled(const std::vector<OrderedTaskPoint*> &_tps,
                         const unsigned _activeTaskPoint,
                         const GlidePolar &_gp);

private:
  virtual GlideResult tp_solution(const unsigned i,
                                   const AIRCRAFT_STATE &aircraft, 
                                   double minH) const;
  virtual double get_min_height(const AIRCRAFT_STATE &aircraft) const {
    return aircraft.Altitude;
  }
  virtual const AIRCRAFT_STATE get_aircraft_start(const AIRCRAFT_STATE &aircraft) const;
};

#endif

