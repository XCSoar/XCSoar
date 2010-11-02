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
  friend class PrintHelper;

  /** 
   * Base constructor.
   * 
   * @param _contest Contests that shall be used
   * @param _result ContestRules to write result to
   * @param trace_full Trace object reference
   * containing full flight history for scanning
   * @param trace_sprint Trace object reference
   * containing 2.5 hour flight history for scanning
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
   * Update internal states (non-essential) for housework,
   * or where functions are slow and would cause loss to real-time performance.
   *
   * @return True if internal state changed
   */
  bool update_idle();

  /** 
   * Reset the task (as if never flown)
   */
  void reset();

  /**
   * Retrieve contest solution vector
   *
   * @return Vector of trace points selected for Contest
   */
  gcc_pure
  const TracePointVector& get_contest_solution() const;

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
};

#endif
