#ifndef TASKEVENTS_HPP
#define TASKEVENTS_HPP

class TaskPoint;

class TaskEvents 
{
public:
/** 
 * Called when the aircraft enters a turnpoint observation zone
 * 
 * @param tp The turnpoint entered
 */
  virtual void transition_enter(const TaskPoint& tp) const;

/** 
 * Called when the aircraft exits a turnpoint observation zone
 * 
 * @param tp The turnpoint the aircraft has exited
 */
  virtual void transition_exit(const TaskPoint &tp) const;

/** 
 * Called when auto-advance has changed the active
 * task point in an ordered task
 * 
 * @param tp The turnpoint that is now active after auto-advance
 * @param i The task sequence number after auto-advance
 */
  virtual void active_advanced(const TaskPoint &tp, const int i) const;

/** 
 * Called when a taskpoint was altered internally.
 * This can happen when an AbortTask determines a better task
 * point is available, or (not yet impelemented) the glider
 * enters a different start point for multiple start points.
 * 
 * @param tp The new active taskpoint
 */
  virtual void active_changed(const TaskPoint &tp) const;

/** 
 * Called when a task is invalid due to improper construction
 * (e.g. no finish point etc) 
 * 
 * @param error Text of error message
 */
  virtual void construction_error(const char* error) const;

};

#endif
