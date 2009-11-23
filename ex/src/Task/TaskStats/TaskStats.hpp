#ifndef TASKSTATS_HPP
#define TASKSTATS_HPP

#include "ElementStat.hpp"

class TaskStats 
{
public:
/** 
 * Constructor.  Initialises all to zero.
 * 
 */
  TaskStats():
    Time(0.0),
    cruise_efficiency(1.0),
    glide_required(0.0),
    mc_best(0.0),
    distance_nominal(0.0),
    distance_max(0.0),
    distance_min(0.0),
    distance_scored(0.0)
    {};

  ElementStat total;
  ElementStat current_leg;

  double Time;

  // calculated values
  double glide_required;
  double cruise_efficiency;
  double mc_best;

  double distance_nominal;
  double distance_max;
  double distance_min;
  double distance_scored;

/** 
 * Reset each element (for incremental speeds).
 * 
 */
  void reset();

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const TaskStats& ts);
#endif
};


#endif
