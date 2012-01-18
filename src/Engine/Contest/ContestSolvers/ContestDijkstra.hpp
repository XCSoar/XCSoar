/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Util/Serial.hpp"
#include "AbstractContest.hpp"
#include "Task/Tasks/PathSolvers/NavDijkstra.hpp"

#include <assert.h>

/**
 * Abstract class for contest searches using dijkstra algorithm
 *
 * These algorithms are designed for online/realtime use, and as such
 * expect solve() to be called during the simulation as time advances.
 *
 *
 */
class ContestDijkstra:
  public AbstractContest,
  public NavDijkstra
{
  /**
   * This attribute tracks Trace::GetModifySerial().  It is updated
   * when a new copy of the master Trace is obtained, and is used to
   * check if that copy should be replaced with a new one.
   */
  Serial modify_serial;

  /**
   * Do an incremental analysis, attempting to improve the result in
   * each iteration?  If set, then only the last point is considered
   * as finish point, and start points are selected according to this.
   */
  bool incremental;

  bool trace_dirty;

  TracePointVector trace; // working trace for solver

protected:
  /** Number of points in current trace set */
  unsigned n_points;

  /** Weightings applied to each leg distance */
  unsigned m_weightings[MAX_STAGES];

  ContestTraceVector best_solution;

public:
  /**
   * Constructor
   *
   * @param _trace Trace object reference to use for solving
   * @param n_legs Maximum number of legs in Contest task
   * @param finish_alt_diff Maximum height loss from start to finish (m)
   */
  ContestDijkstra(const Trace &_trace, 
                  const unsigned n_legs,
                  const unsigned finish_alt_diff = 1000);

  void SetIncremental(bool _incremental) {
    incremental = _incremental;
  }

  bool Score(ContestResult &result);

  virtual void CopySolution(ContestTraceVector &vec) const;

protected:
  virtual fixed CalcDistance() const;
  virtual fixed CalcScore() const;
  virtual fixed CalcTime() const;

public:
  /**
   * Reset the optimiser as if never flown
   */
  virtual void Reset();

  /**
   * Update the solver.  The solver is incremental, so this method can
   * be safely called every time step.
   *
   * @return True if solver completed in this call
   */
  virtual bool Solve(bool exhaustive);

protected:
  gcc_pure
  const TracePoint &GetPoint(unsigned i) const {
    assert(i < n_points);

    return trace[i];
  }

  gcc_pure
  const TracePoint &GetPointFast(const ScanTaskPoint sp) const {
    assert(sp.GetPointIndex() < n_points);
    return trace[sp.GetPointIndex()];
  }

  /** Update working trace from master --- never to be done during a solution! */
  virtual void update_trace();

  void clear_trace();

  virtual void add_edges(ScanTaskPoint curNode);

  /**
   * Retrieve weighting of specified leg
   * @param index Index of leg
   * @return Weighting of leg
   */
  gcc_pure
  unsigned get_weighting(const unsigned index) const {
    assert(num_stages <= MAX_STAGES);
    assert(index + 1 < num_stages);

    return m_weightings[index];
  }

  /**
   * Perform actions required at start of new search
   */
  virtual void start_search();

  virtual bool SaveSolution();

  /** 
   * Distance function for free point
   * 
   * @param curNode Destination node
   * @param currentLocation Origin location
   * 
   * @return Distance (flat) from origin to destination
   */
  gcc_pure
  unsigned distance(const ScanTaskPoint curNode,
                    const SearchPoint &currentLocation) const {
    return GetPointFast(curNode).flat_distance(currentLocation);
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
  unsigned distance(const ScanTaskPoint s1, const ScanTaskPoint s2) const {
    return GetPointFast(s1).flat_distance(GetPointFast(s2));
  }

private:
  virtual void add_start_edges();

  gcc_pure
  bool master_is_updated() const;
};

#endif
