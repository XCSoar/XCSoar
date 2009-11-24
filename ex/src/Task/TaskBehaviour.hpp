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

  bool optimise_targets_range;
  bool optimise_targets_bearing;
  bool auto_mc;
  bool calc_cruise_efficiency;
  double aat_min_time;
  double safety_height_terrain;
  double safety_height_arrival;
  double start_max_speed;

};

#endif
