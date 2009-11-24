#ifndef ABORTTASK_H
#define ABORTTASK_H

#include "AbstractTask.hpp"
#include <vector>
#include "Waypoint/Waypoints.hpp"

/**
 * Abort task provides automatic management of a sorted list of task points
 * that are reachable or close to reachable, and landable (with airfields preferred).
 */
class AbortTask : public AbstractTask 
{
public:
  /** 
   * Base constructor.
   * 
   * @param te Task events callback class (shared among all tasks) 
   * @param tb Global task behaviour settings
   * @param ta Advance mechanism used for advancable tasks
   * @param gp Global glide polar used for navigation calculations
   * 
   * @return Initialised object (with nothing in task)
   */
  AbortTask(const TaskEvents &te, 
            const TaskBehaviour &tb,
            TaskAdvance &ta,
            GlidePolar &gp,
            const Waypoints &wps);
  ~AbortTask();

/** 
 * Retrieves the active task point sequence.
 * 
 * @return Index of active task point sequence
 */
  TaskPoint* getActiveTaskPoint() const;

/** 
 * Set active task point index
 * 
 * @param index Desired active index of task sequence
 */
  void setActiveTaskPoint(unsigned index);

/** 
 * Update internal states when aircraft state advances.
 * This performs a scan of reachable waypoints.
 * 
 * \todo
 * - check tracking of active waypoint
 *
 * @param state_now Aircraft state at this time step
 * @param full_update Force update due to task state change
 *
 * @return True if internal state changes
 */
  virtual bool update_sample(const AIRCRAFT_STATE &state_now, 
                             const bool full_update);

protected:

/** 
 * Test whether (and how) transitioning into/out of task points should occur, typically
 * according to task_advance mechanism.  This also may call the task_event callbacks.
 * 
 * @param state_now Aircraft state at this time step
 * @param state_last Aircraft state at previous time step
 * 
 * @return True if transition occurred
 */
  virtual bool check_transitions(const AIRCRAFT_STATE& state_now, 
                                 const AIRCRAFT_STATE& state_last);

private:
/** 
 * Clears task points in list
 * 
 */
  void clear();

/** 
 * Check whether abort task list is full
 * 
 * @return True if no more task points can be added
 */
  bool task_full() const;

/** 
 * Calculate distance to search for landable waypoints for aircraft.
 * 
 * @param state_now Aircraft state
 * 
 * @return Distance (m) of approximate glide range of aircraft
 */
  double abort_range(const AIRCRAFT_STATE &state_now);

/** 
 * Propagate changes to safety glide polar from global glide polar. 
 * 
 */
  void update_polar();

/** 
 * Fill abort task list with candidate waypoints given a list of
 * waypoints satisfying approximate range queries.  Can be used
 * to add airfields only, or landpoints.
 *
 * @param state_now Aircraft state
 * @param approx_waypoints List of candidate waypoints
 * @param only_airfield If true, only add waypoints that are airfields.
 */
  void fill_reachable(const AIRCRAFT_STATE &state,
                      std::vector < Waypoint > &approx_waypoints,
                      const bool only_airfield);

  std::vector<TaskPoint*> tps;
  unsigned active_waypoint;
  const Waypoints &waypoints;
  GlidePolar polar_safety;

  typedef std::pair<Waypoint,double> WP_ALT;

  /**
   * Function object used to rank waypoints by arrival altitude
   */
  struct Rank : public std::binary_function<WP_ALT, WP_ALT, bool> {
    bool operator()(const WP_ALT& x, const WP_ALT& y) const {
      return x.second > y.second;
    }
  };

public:
#ifdef DO_PRINT
  virtual void print(const AIRCRAFT_STATE &location);
#endif

/** 
 * Accept a task point visitor; makes the visitor visit
 * all TaskPoint in the task
 * 
 * @param visitor Visitor to accept
 */
  void Accept(TaskPointVisitor& visitor) const;
  DEFINE_VISITABLE()
};

#endif //ABORTTASK_H
