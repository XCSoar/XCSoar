#ifndef ABORTTASK_H
#define ABORTTASK_H

#include "AbstractTask.hpp"
#include <vector>
#include "Navigation/TaskProjection.hpp"
#include "Waypoint/Waypoints.hpp"

class AbortTask : public AbstractTask 
{
public:
  AbortTask(const TaskEvents &te, 
            const TaskBehaviour &tb,
            const TaskProjection &tp,
            TaskAdvance &ta,
            GlidePolar &gp,
            const Waypoints &wps);
  ~AbortTask();

/** 
 * Retrieves the active task point sequence.
 * 
 * @return Index of active task point sequence
 */
  TaskPoint* getActiveTaskPoint();

/** 
 * Set active task point index
 * 
 * @param desired Desired active index of task sequence
 */
  void setActiveTaskPoint(unsigned index);

/** 
 * Update internal states when aircraft state advances.
 * This performs a scan of reachable waypoints.
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
  const TaskProjection &task_projection;
  unsigned active_waypoint;
  const Waypoints &waypoints;
  GlidePolar polar_safety;

  /** @link dependency */
  /*#  Rank lnkRank; */
public:
#ifdef DO_PRINT
  virtual void print(const AIRCRAFT_STATE &location);
#endif

  void Accept(TaskPointVisitor& visitor) const;
  DEFINE_VISITABLE()
};

#endif //ABORTTASK_H
