#ifndef ONLINE_CONTEST_HPP
#define ONLINE_CONTEST_HPP

#include "Navigation/Aircraft.hpp"
#include "Navigation/TracePoint.hpp"
#include "Navigation/TaskProjection.hpp"
#include <vector>

#include "PathSolvers/OLCSprint.hpp"
#include "PathSolvers/OLCFAI.hpp"
#include "PathSolvers/OLCClassic.hpp"
#include "PathSolvers/Contests.hpp"

class TaskPoint;
class CommonStats;
class Trace;

/**
 * Special task holder for Online Contest calculations
 */
class ContestManager {
public:

  /** 
   * Base constructor.
   * 
   * @param olc_rules Contests that shall be used
   * @param stats Common stats to write OLC info to
   * @param trace_full Trace object containing full flight history for scanning
   * @param trace_sprint Trace object containing 2.5 hour flight history for scanning
   * 
   */
  ContestManager(const Contests _contest,
                ContestResult &_result, const Trace &trace_full,
                const Trace &trace_sprint);

  void set_contest(Contests _contest) {
    contest = _contest;
  }

  /**
   * Update internal states when aircraft state advances.
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
  bool update_idle();

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
  gcc_pure
  const TracePointVector& get_trace_points() const;

  /**
   * Retrieve olc solution vector
   *
   * @return Vector of trace points selected for OLC
   */
  gcc_pure
  const TracePointVector& get_olc_points() const;

private:
  Contests contest;
  ContestResult &result;

  const Trace &trace_full;
  const Trace &trace_sprint;

  TracePointVector trace_points_full;
  TracePointVector trace_points_sprint;

  TracePointVector solution;

  bool run_olc(ContestDijkstra& dijkstra);

  OLCSprint olc_sprint;
  OLCFAI olc_fai;
  OLCClassic olc_classic;

  void update_trace();

  bool update_trace_sample(const AIRCRAFT_STATE &state, TracePointVector& vec);

public:
#ifdef DO_PRINT
  void print() const;
#endif
};

#endif
