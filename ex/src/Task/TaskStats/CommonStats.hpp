#ifndef COMMON_STATS_HPP
#define COMMON_STATS_HPP

#include "Math/fixed.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

class CommonStats 
{
public:
/** 
 * Constructor, initialises all to zero
 * 
 */
  CommonStats();
  
  bool landable_reachable; /**< Whether the task found landable reachable waypoints (aliases abort) */
  bool task_finished; /**< Whether the task is finished (aliases ordered task) */
  fixed aat_time_remaining; /**< Time (s) until assigned minimum time is achieved */
  fixed aat_remaining_speed; /**< Speed to achieve remaining task in minimum assigned time (m/s), negative if already beyond minimum time */ 
  fixed task_time_remaining; /**< Time (s) remaining for ordered task */
  fixed task_time_elapsed; /**< Time (s) elapsed for ordered task */
  GeoVector vector_home; /**< Vector to home waypoint */

/** 
 * Reset the task as if never flown
 * 
 */
  void reset();
};

#endif
