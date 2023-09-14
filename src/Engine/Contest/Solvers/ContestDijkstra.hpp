// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AbstractContest.hpp"
#include "PathSolvers/NavDijkstra.hpp"
#include "TraceManager.hpp"

#include <cassert>

class Trace;

/**
 * Abstract class for contest searches using dijkstra algorithm
 *
 * These algorithms are designed for online/realtime use, and as such
 * expect solve() to be called during the simulation as time advances.
 *
 *
 */
class ContestDijkstra : public AbstractContest, protected NavDijkstra<>, public TraceManager {
  /**
   * Is this a contest that allows continuous analysis?
   */
  const bool continuous;

  /**
   * Do an incremental analysis, attempting to improve the result in
   * each iteration?  If set, then only the last point is considered
   * as finish point, and start points are selected according to this.
   */
  bool incremental = false;

  /**
   * Did the last Dijkstra search finish (even if without a valid
   * solution)?  This means the Dijkstra object still contains valid
   * data, and may be resumed with new data.  This flag gets cleared
   * when Trace or Dijkstra get cleared.
   */
  bool finished;

  /**
   * The last solution.  Use only if Solve() has returned VALID.
   */
  ContestTraceVector solution;

  /**
   * The required minimum leg distance.
   */
  const double min_distance;

protected:
  /**
   * The index of the first finish candidate.  During incremental
   * scan, only the new trace points are considered.
   */
  unsigned first_finish_candidate;

  /** Weightings applied to each leg distance */
  unsigned stage_weights[MAX_STAGES];

public:
  /**
   * Constructor
   *
   * @param _trace Trace object reference to use for solving
   * @param n_legs Maximum number of legs in Contest task
   * @param finish_alt_diff Maximum height loss from start to finish (m)
   * @param _min_distance The minimum required leg distance (m)
   */
  ContestDijkstra(const Trace &_trace,
                  bool continuous,
                  const unsigned n_legs,
                  const unsigned finish_alt_diff = 1000,
                  const double _min_distance = 0.0) noexcept;

  void SetIncremental(bool _incremental) noexcept {
    incremental = _incremental;
  }

protected:
  bool IsIncremental() const noexcept {
    return incremental;
  }

  [[gnu::pure]]
  const TracePoint &GetPoint(const ScanTaskPoint sp) const noexcept {
    return TraceManager::GetPoint(sp.GetPointIndex());
  }

  void AddEdges(ScanTaskPoint origin, unsigned first_point) noexcept;

  /**
   * Restart the solver with the new points added by
   * UpdateTraceTail().
   *
   * @param first_point the first point that was added
   */
  void AddIncrementalEdges(unsigned first_point) noexcept;

  /**
   * Retrieve weighting of specified leg
   * @param index Index of leg
   * @return Weighting of leg
   */
  [[gnu::pure]]
  unsigned GetStageWeight(const unsigned index) const noexcept {
    assert(num_stages <= MAX_STAGES);
    assert(index + 1 < num_stages);

    return stage_weights[index];
  }

  /**
   * Distance function for edges
   *
   * @param s1 Origin node
   * @param s2 Destination node
   *
   * @return Distance (flat) from origin to destination
   */
  [[gnu::pure]]
  value_type CalcEdgeDistance(const ScanTaskPoint s1,
                              const ScanTaskPoint s2) const noexcept {
    return GetPoint(s1).FlatDistanceTo(GetPoint(s2));
  }

  bool Link(const ScanTaskPoint node, const ScanTaskPoint parent,
            value_type value) noexcept {
    return NavDijkstra<>::Link(node, parent, DIJKSTRA_MINMAX_OFFSET - value);
  }

private:
  [[gnu::pure]]
  bool CheckMinDistance(const GeoPoint &origin,
                        const GeoPoint &destination) const noexcept {
    if (min_distance <= 0)
      /* no minimum distance, don't bother calculating the actual
         distance */
      return true;

    return origin.Distance(destination) >= min_distance;
  }


  bool SaveSolution() noexcept;

protected:
  /**
   * Update working trace from master.
   *
   * @param force disable lazy updates, force the trace to be up to
   * date before returning
   */
  void UpdateTrace(bool force) noexcept override;

  /**
   * Perform actions required at start of new search
   */
  virtual void StartSearch() noexcept;
  virtual void AddStartEdges() noexcept;
  virtual ContestResult CalculateResult(const ContestTraceVector &solution) const noexcept;

public:
  /* public virtual methods from AbstractContest */
  SolverResult Solve(bool exhaustive) noexcept override;
  void Reset() noexcept override;

protected:
  /* protected virtual methods from AbstractContest */
  ContestResult CalculateResult() const noexcept override;
  const ContestTraceVector &GetCurrentPath() const noexcept override;

protected:
  /* virtual methods from NavDijkstra */
  void AddEdges(ScanTaskPoint curNode) noexcept override;
};
