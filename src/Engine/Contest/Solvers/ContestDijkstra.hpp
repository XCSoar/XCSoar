/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef OLC_DIJKSTRA_HPP
#define OLC_DIJKSTRA_HPP

#include "AbstractContest.hpp"
#include "PathSolvers/NavDijkstra.hpp"
#include "TraceManager.hpp"

#include <assert.h>

class Trace;

/**
 * Abstract class for contest searches using dijkstra algorithm
 *
 * These algorithms are designed for online/realtime use, and as such
 * expect solve() to be called during the simulation as time advances.
 *
 *
 */
class ContestDijkstra : public AbstractContest, protected NavDijkstra, public TraceManager {
  /**
   * Is this a contest that allows continuous analysis?
   */
  const bool continuous;

  /**
   * Do an incremental analysis, attempting to improve the result in
   * each iteration?  If set, then only the last point is considered
   * as finish point, and start points are selected according to this.
   */
  bool incremental;

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
   */
  ContestDijkstra(const Trace &_trace,
                  bool continuous,
                  const unsigned n_legs,
                  const unsigned finish_alt_diff = 1000);

  void SetIncremental(bool _incremental) {
    incremental = _incremental;
  }

protected:
  bool IsIncremental() const {
    return incremental;
  }

  gcc_pure
  const TracePoint &GetPoint(const ScanTaskPoint sp) const {
    return TraceManager::GetPoint(sp.GetPointIndex());
  }

  void AddEdges(ScanTaskPoint origin, unsigned first_point);

  /**
   * Restart the solver with the new points added by
   * UpdateTraceTail().
   *
   * @param first_point the first point that was added
   */
  void AddIncrementalEdges(unsigned first_point);

  /**
   * Retrieve weighting of specified leg
   * @param index Index of leg
   * @return Weighting of leg
   */
  gcc_pure
  unsigned GetStageWeight(const unsigned index) const {
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
  gcc_pure
  unsigned CalcEdgeDistance(const ScanTaskPoint s1,
                            const ScanTaskPoint s2) const {
    return GetPoint(s1).FlatDistanceTo(GetPoint(s2));
  }

  bool Link(const ScanTaskPoint node, const ScanTaskPoint parent,
            unsigned value) {
    return NavDijkstra::Link(node, parent, DIJKSTRA_MINMAX_OFFSET - value);
  }

private:
  bool SaveSolution();

protected:
  /**
   * Update working trace from master.
   *
   * @param force disable lazy updates, force the trace to be up to
   * date before returning
   */
  void UpdateTrace(bool force) override;

  /**
   * Perform actions required at start of new search
   */
  virtual void StartSearch();
  virtual void AddStartEdges();
  virtual ContestResult CalculateResult(const ContestTraceVector &solution) const;

public:
  /* public virtual methods from AbstractContest */
  SolverResult Solve(bool exhaustive) override;
  void Reset() override;

protected:
  /* protected virtual methods from AbstractContest */
  ContestResult CalculateResult() const override;
  void CopySolution(ContestTraceVector &vec) const override;

protected:
  /* virtual methods from NavDijkstra */
  void AddEdges(ScanTaskPoint curNode) override;
};

#endif
