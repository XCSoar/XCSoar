#ifndef TASK_BEHAVIOUR_HPP
#define TASK_BEHAVIOUR_HPP

/**
 *  Class defining options for task system.
 *  Typical uses might be default values, and simple aspects of task behaviour.
 */
class TaskBehaviour 
{
public:

/** 
 * Constructor, sets default task behaviour
 * 
 */
  TaskBehaviour();

/** 
 * Convenience function (used primarily for testing) to disable
 * all expensive task behaviour functions.
 */
  void all_off();

/** 
 * Check whether aircraft speed is within start speed limits
 * 
 * @param state Aircraft state
 * 
 * @return True if within limits
 */
  bool check_start_speed(const AIRCRAFT_STATE &state) const;

  bool optimise_targets_range; /**< Option to enable positionining of AAT targets to achieve desired AAT minimum task time */
  bool optimise_targets_bearing; /**< Option to enable positioning of AAT targets at optimal point on isoline */
  bool auto_mc; /**< Option to enable calculation and setting of auto MacCready */
  bool calc_cruise_efficiency; /**< Option to enable calculation of cruise efficiency */
  double aat_min_time; /**< Desired AAT minimum task time (s) */
  double safety_height_terrain; /**< Minimum height above terrain for arrival height at non-landable waypoint (m) */
  double safety_height_arrival; /**< Minimum height above terrain for arrival height at landable waypoint (m) */
  double start_max_speed; /**< Maximum ground speed (m/s) allowed in start sector */

};

#endif
