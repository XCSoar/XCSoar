#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include "Util/Serialisable.hpp"
#include "Tasks/TaskInterface.hpp"
#include "Tasks/AbortTask.hpp"
#include "Tasks/GotoTask.hpp"
#include "Tasks/OrderedTask.hpp"
#include "TaskStats/TaskStats.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "TaskEvents.hpp"
#include "TaskBehaviour.hpp"

#include "Factory/FAITaskFactory.hpp"
#include "Factory/AATTaskFactory.hpp"
#include "Factory/MixedTaskFactory.hpp"

class Waypoints;
class TaskVisitor;

class TaskManager: 
 public TaskInterface,
 public Serialisable
{
public:
/** 
 * Constructor for task manager
 * 
 * @param te Task events callback object
 * @param tb Task behaviour options
 * @param gp Glide polar used for task calculations
 * @param wps Waypoint system for use by AbortTask
 * 
 * @return Initialised object
 */
  TaskManager(const TaskEvents &te,
              const TaskBehaviour &tb,
              GlidePolar &gp,
              const Waypoints &wps);

/** 
 * Sets active taskpoint sequence for active task
 * 
 * @param index Sequence number of task point
 */
    virtual void setActiveTaskPoint(unsigned index);

/** 
 * Accessor for active taskpoint sequence for active task
 * 
 * @return Sequence number of task point
 */
  virtual unsigned getActiveTaskPointIndex() const;

/** 
 * Accessor of current task point of active task
 * 
 * @return TaskPoint of active task point, and 0 if no active task
 */
  virtual TaskPoint* getActiveTaskPoint() const;

/** 
 * Get a random point in the task OZ (for testing simulation route)
 * 
 * @param index Index sequence of task point
 * @param mag proportional magnitude of error from center (0-1)
 * 
 * @return Location of point
 */
  GEOPOINT random_point_in_task(const unsigned index, const double mag=1.0) const;

  enum TaskMode_t {
    MODE_NULL=0,
    MODE_ORDERED,
    MODE_ABORT,
    MODE_GOTO
  };

  enum Factory_t {
    FACTORY_FAI=0,
    FACTORY_AAT,
    FACTORY_MIXED
  };

  /** 
   * Reset the ordered task (as if never flown)
   * 
   */
  void reset();

/** 
 * Set active task to abort mode.
 * 
 */
  void abort();

/** 
 * Sets active task to ordered task (or goto if none exists) after
 * goto or aborting.
 * 
 */
  void resume();

/** 
 * Sets active task to go to mode, to specified waypoint
 * 
 * @param wp Waypoint to go to
 */
  void do_goto(const Waypoint & wp);

/** 
 * Updates internal state of task given new aircraft.
 * Only essential calculations are performed here;
 * other calculations and housekeeping may be performed
 * by update_idle
 * 
 * @param state_now Current aircraft state
 * @param state_last Aircraft state at last update 
 * @return True if internal state changed
 */
  virtual bool update(const AIRCRAFT_STATE &state_now, 
                      const AIRCRAFT_STATE &state_last);

/** 
 * Updates internal state of task to produce
 * auxiliary information or to perform slow house-keeping
 * functions that are non-essential.
 * 
 * @param state Current aircraft state
 * 
 * @return True if internal state changed
 */
  virtual bool update_idle(const AIRCRAFT_STATE &state);

/** 
 * Accessor for statistics of active task
 * 
 * @return Statistics of active task
 */
  virtual const TaskStats& get_stats() const;

/** 
 * Size of OrderedTask
 * 
 * @return Number of taskpoints in OrderedTask
 */
  unsigned get_task_size() const;

/** 
 * Check whether ordered task is valid
 * 
 * @return True if task is valid
 */
  bool check_task() const;


#ifdef DO_PRINT
  virtual void print(const AIRCRAFT_STATE &location);
#endif

/** 
 * Accessor for factory system for constructing tasks
 * 
 * @return Factory
 */
  AbstractTaskFactory* get_factory() const {
    return active_factory;
  }

/** 
 * Set type of task factory to be used for constructing tasks
 * 
 * @param _factory Type of task
 * 
 * @return Type of task
 */
  Factory_t set_factory(const Factory_t _factory);

private:
  const TaskStats null_stats;

  TaskMode_t set_mode(const TaskMode_t mode);

  AbstractTask* active_task;
  AbstractTaskFactory* active_factory;

  TaskMode_t mode;
  Factory_t factory_mode;

  /** @link aggregation */
  FAITaskFactory factory_fai;

  /** @link aggregation */
  AATTaskFactory factory_aat;

  /** @link aggregation */
  MixedTaskFactory factory_mixed;

  /** @link aggregation */
  OrderedTask task_ordered;

  /** @link aggregation */
  GotoTask task_goto;
  
  /** @link aggregation */
  AbortTask task_abort;
  
  /** @link aggregation */
  TaskAdvance task_advance;
  
  const TaskBehaviour &task_behaviour;
  
public:
  void Accept(BaseVisitor& visitor) const;
};
#endif //TASKMANAGER_H
