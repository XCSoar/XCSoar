#ifndef ONLINE_CONTEST_HPP
#define ONLINE_CONTEST_HPP

#include "Util/GenericVisitor.hpp"
#include "Navigation/Aircraft.hpp"
#include "Navigation/TracePoint.hpp"
#include "Navigation/TaskProjection.hpp"
#include <vector>

#include "PathSolvers/OLCSprint.hpp"
#include "PathSolvers/OLCFAI.hpp"
#include "PathSolvers/OLCClassic.hpp"

class TaskPoint;
class TaskEvents;
class TaskBehaviour;
class TaskPointVisitor;
class TaskPointConstVisitor;
class GlidePolar;
class CommonStats;
class Trace;

/**
 * Special task holder for Online Contest calculations
 */
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
   * @param stats Common stats to write OLC info to
   * @param trace_full Trace object containing full flight history for scanning
   * @param trace_sprint Trace object containing 2.5 hour flight history for scanning
   * 
   */
  OnlineContest(const TaskEvents &te, 
                const TaskBehaviour &tb,
                const GlidePolar &gp,
                CommonStats &stats,
                const Trace &trace_full,
                const Trace &trace_sprint);

/** 
 * Update internal states when aircraft state advances.
 * This performs a scan of reachable waypoints.
 * 
 * \todo
 * - check tracking of active waypoint
 *
 * @param state Aircraft state at this time step
 *
 * @return True if internal state changes
 */
  bool update_sample(const AIRCRAFT_STATE &state);

/** 
 * Update internal states (non-essential) for housework, or where functions are slow
 * and would cause loss to real-time performance.
 * 
 * @param state Aircraft state at this time step
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
 * @return Vector of all trace points 
 */
  const TracePointVector& get_trace_points(const bool full_trace=true) const;

/** 
 * Retrieve olc solution vector
 * 
 * @return Vector of trace points selected for OLC
 */
  const TracePointVector& get_olc_points() const;

private:
  const TaskEvents &m_task_events;
  const TaskBehaviour &m_task_behaviour;
  const GlidePolar &m_glide_polar;
  CommonStats &common_stats;

  const Trace &m_trace_full;
  const Trace &m_trace_sprint;

  TracePointVector m_trace_points_full;
  TracePointVector m_trace_points_sprint;

  TracePointVector m_solution;

  bool run_olc(OLCDijkstra& dijkstra);

  OLCSprint olc_sprint;
  OLCFAI olc_fai;
  OLCClassic olc_classic;

  void update_trace();

  bool update_trace_sample(const AIRCRAFT_STATE &state,
                           TracePointVector& vec);

public:
#ifdef DO_PRINT
  void print() const;
#endif

/** 
 * Accept a task point visitor; makes the visitor visit
 * all TaskPoint in the task
 * 
 * @param visitor Visitor to accept
 * @param reverse Visit task points in reverse order 
 *
 * \todo reverse not implemented yet
 */
  void tp_CAccept(TaskPointConstVisitor& visitor, const bool reverse=false) const;
  void tp_Accept(TaskPointVisitor& visitor, const bool reverse=false) {};

  DEFINE_VISITABLE()
};

#endif
