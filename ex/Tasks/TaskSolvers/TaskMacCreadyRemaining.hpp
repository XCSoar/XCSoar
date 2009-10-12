#ifndef TASKMACCREADYREMAINING_HPP
#define TASKMACCREADYREMAINING_HPP

#include "TaskMacCready.hpp"

class TaskMacCreadyRemaining: 
  public TaskMacCready
{
public:
  TaskMacCreadyRemaining(const std::vector<OrderedTaskPoint*> &_tps,
                         const unsigned _activeTaskPoint,
                         const double _mc);

  void set_range(const double tp);

protected:
  virtual GLIDE_RESULT tp_solution(const unsigned i,
                                   const AIRCRAFT_STATE &aircraft, 
                                   double minH) const;
  virtual double get_min_height(const AIRCRAFT_STATE &aircraft) const {
    return 0.0;
  }
  virtual const AIRCRAFT_STATE get_aircraft_start(const AIRCRAFT_STATE &aircraft) const;

};

#endif
