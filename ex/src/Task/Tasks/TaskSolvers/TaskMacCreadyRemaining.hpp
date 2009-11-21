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
  TaskMacCreadyRemaining(const std::vector<OrderedTaskPoint*> &_tps,
                         const unsigned _activeTaskPoint,
                         const GlidePolar _gp);
  TaskMacCreadyRemaining(TaskPoint* tp,
                         const GlidePolar _gp);

  void set_range(const double tp, const bool force_current);

protected:
  virtual GlideResult tp_solution(const unsigned i,
                                   const AIRCRAFT_STATE &aircraft, 
                                   double minH) const;
  virtual double get_min_height(const AIRCRAFT_STATE &aircraft) const {
    return 0.0;
  }
  virtual const AIRCRAFT_STATE get_aircraft_start(const AIRCRAFT_STATE &aircraft) const;

};

#endif
