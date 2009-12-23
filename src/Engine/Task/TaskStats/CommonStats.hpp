#ifndef COMMON_STATS_HPP
#define COMMON_STATS_HPP

#include "Math/fixed.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include <set>

typedef std::set<unsigned> WaypointIdSet;

class Waypoint;

/** 
 * Task statistics that are common across all managed tasks.
 * This is used for statistics for which it makes no sense to
 * have per-task instances, and where access to certain statistics
 * is required whatever mode the task manager is in.
 */
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
  fixed aat_speed_remaining; /**< Speed to achieve remaining task in minimum assigned time (m/s), negative if already beyond minimum time */ 
  fixed aat_speed_max; /**< Average speed over max task at minimum assigned time (m/s) */
  fixed aat_speed_min; /**< Average speed over min task at minimum assigned time (m/s) */
  fixed task_time_remaining; /**< Time (s) remaining for ordered task */
  fixed task_time_elapsed; /**< Time (s) elapsed for ordered task */
  GeoVector vector_home; /**< Vector to home waypoint */
  bool mode_abort; /**< Whether task is abort mode */
  bool mode_ordered; /**< Whether task is ordered mode */
  bool mode_goto; /**< Whether task is goto mode */
  bool ordered_valid; /**< Whether ordered task is valid */

  bool active_has_next; /**< Is there a tp after this */
  bool active_has_previous; /**< Is there a tp before this */
  bool next_is_last; /**< Is next turnpoint the final */
  bool previous_is_first; /**< Is previous turnpoint the first */

  fixed V_block; /**< Block speed to fly */
  fixed V_dolphin; /**< Dolphin speed to fly */

  WaypointIdSet waypoints_in_task; /**< List of waypoints by id that are managed in the task */

/** 
 * Clears the set of waypoints listed as in the task
 * 
 */
  void clear_waypoints_in_task();

/** 
 * Mark a waypoint as being in the task
 * 
 * @param wp Waypoint to add
 */
  void append_waypoint_in_task(const Waypoint& wp);

/** 
 * Test whether a waypoint is in the task
 * 
 * @param wp Waypoint to test
 * @return True if in task
 */
  bool is_waypoint_in_task(const Waypoint& wp) const;

/** 
 * Reset the task as if never flown
 * 
 */
  void reset();

};

#endif
