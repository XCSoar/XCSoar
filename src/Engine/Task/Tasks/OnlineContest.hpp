#ifndef ONLINE_CONTEST_HPP
#define ONLINE_CONTEST_HPP

#include "Util/GenericVisitor.hpp"
#include "Navigation/Aircraft.hpp"
#include "Navigation/SearchPointVector.hpp"
#include "Navigation/TaskProjection.hpp"
#include <vector>

class TaskPoint;
class TaskEvents;
class TaskBehaviour;
class TaskPointVisitor;
class GlidePolar;

class OnlineContest:
  public BaseVisitable<> 
{
public:

  /** 
   * Base constructor.
   * 
   * @param te Task events callback class (shared among all tasks) 
   * @param tb Global task behaviour settings
   * @param gp Global glide polar used for navigation calculations
   * 
   */
  OnlineContest(const TaskEvents &te, 
                const TaskBehaviour &tb,
                const GlidePolar &gp);

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
  bool update_sample(const AIRCRAFT_STATE &state_now, 
                     const bool full_update);

  /** 
   * Reset the task (as if never flown)
   * 
   */
  void reset();

/** 
 * Retrieve interior sample polygon (pure).
 * 
 * @return Vector of sample points 
 */
  const SearchPointVector& get_sample_points() const;

private:
  const TaskEvents &m_task_events;
  const TaskBehaviour &m_task_behaviour;
  const GlidePolar &m_glide_polar;
  TaskProjection m_task_projection;

  SearchPointVector m_sampled_points;

  // note this is copied from OrderedTask.cpp
  bool distance_is_significant(const GEOPOINT &location,
                               const GEOPOINT &location_last) const;

  void thin_samples();

public:
/** 
 * Accept a task point visitor; makes the visitor visit
 * all TaskPoint in the task
 * 
 * @param visitor Visitor to accept
 * @param reverse Visit task points in reverse order 
 *
 * \todo reverse not implemented yet
 */
  void Accept(TaskPointVisitor& visitor, const bool reverse=false) const;

  DEFINE_VISITABLE()
};

#endif
