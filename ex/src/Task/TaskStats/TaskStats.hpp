#ifndef TASKSTATS_HPP
#define TASKSTATS_HPP

#include "ElementStat.hpp"

/**
 * Container for common task statistics
 */
class TaskStats 
{
public:
/** 
 * Constructor.  Initialises all to zero.
 * 
 */
  TaskStats();

  ElementStat total; /**< Total task statistics */
  ElementStat current_leg; /**< Current (active) leg statistics */

  double Time; /**< Global time (UTC, s) of last update */

  // calculated values
  double glide_required; /**< Calculated glide angle required */
  double cruise_efficiency; /**< Calculated cruise efficiency ratio */
  double mc_best; /**< Best MacCready setting calculated for final glide (m/s) */

  double distance_nominal; /**< Nominal task distance (m) */
  double distance_max; /**< Maximum achievable task distance (m) */
  double distance_min; /**< Minimum achievable task distance (m) */
  double distance_scored; /**< Scored distance (m) */

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
