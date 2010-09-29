#ifndef COMMON_STATS_HPP
#define COMMON_STATS_HPP

#include "Math/fixed.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include <set>

#ifdef DO_PRINT
#include <ostream>
#endif

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
   */
  CommonStats();
  
  /** Whether the task found landable reachable waypoints (aliases abort) */
  bool landable_reachable;
  /** Whether the task is started (aliases ordered task) */
  bool task_started;
  /** Whether the task is finished (aliases ordered task) */
  bool task_finished;
  /** Time (s) until assigned minimum time is achieved */
  fixed aat_time_remaining;
  /**
   * Speed to achieve remaining task in minimum assigned time (m/s),
   * negative if already beyond minimum time
   */
  fixed aat_speed_remaining;
  /** Average speed over max task at minimum assigned time (m/s) */
  fixed aat_speed_max;
  /** Average speed over min task at minimum assigned time (m/s) */
  fixed aat_speed_min;
  /** Time (s) remaining for ordered task */
  fixed task_time_remaining;
  /** Time (s) elapsed for ordered task */
  fixed task_time_elapsed;
  /** Vector to home waypoint */
  GeoVector vector_home;
  /** Whether task is abort mode */
  bool mode_abort;
  /** Whether task is goto mode */
  bool mode_ordered;
  /** Whether task is goto mode */
  bool mode_goto;
  /** Whether ordered task is valid */
  bool ordered_valid;
  /** Whether ordered task has AAT areas */
  bool ordered_has_targets;

  /** Is there a tp after this */
  bool active_has_next;
  /** Is there a tp before this */
  bool active_has_previous;
  /** Is next turnpoint the final */
  bool next_is_last;
  /** Is previous turnpoint the first */
  bool previous_is_first;
  /** index of active tp */
  unsigned active_taskpoint_index;

  /** Block speed to fly */
  fixed V_block;
  /** Dolphin speed to fly */
  fixed V_dolphin;

  /** MC setting at last update (m/s) */
  fixed current_mc;
  /** Risk MC setting (m/s) */
  fixed current_risk_mc;
  /** Bugs setting at last update */
  fixed current_bugs;
  /** Ballast setting at last update */
  fixed current_ballast;

  /** Optimum distance (m) travelled according to OLC rule */
  fixed distance_olc;
  /** Time (s) of optimised OLC path */
  fixed time_olc;
  /** Speed (m/s) of optimised OLC path */
  fixed speed_olc;

  /**
   * Reset the stats as if never flown
   */
  void reset();

  /**
   * Reset the task stats
   */
  void reset_task();

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, const CommonStats& ts);
#endif
};

#endif
