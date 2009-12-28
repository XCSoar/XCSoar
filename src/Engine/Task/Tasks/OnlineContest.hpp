#ifndef ONLINE_CONTEST_HPP
#define ONLINE_CONTEST_HPP

#include "Util/GenericVisitor.hpp"
#include "Navigation/Aircraft.hpp"
#include "Navigation/TracePoint.hpp"
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
 *
 * @return True if internal state changes
 */
  bool update_sample(const AIRCRAFT_STATE &state_now);

/** 
 * Update internal states (non-essential) for housework, or where functions are slow
 * and would cause loss to real-time performance.
 * 
 * @param state_now Aircraft state at this time step
 * 
 * @return True if internal state changed
 */
  bool update_idle(const AIRCRAFT_STATE &state);

  /** 
   * Reset the task (as if never flown)
   * 
   */
  void reset();

/** 
 * Retrieve trace vector
 * 
 * @return Vector of sample points 
 */
  const TracePointVector& get_trace_points() const;

private:
  const TaskEvents &m_task_events;
  const TaskBehaviour &m_task_behaviour;
  const GlidePolar &m_glide_polar;
  TaskProjection m_task_projection;

  TracePointVector m_trace_points;

  bool distance_is_significant(const AIRCRAFT_STATE &state,
                               const TracePoint &state_last) const;

  void thin_trace();

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
