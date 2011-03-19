/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "NavDijkstra.hpp"

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
  public NavDijkstra<TracePoint>
{
public:
  /**
   * Constructor
   *
   * @param _trace Trace object reference to use for solving
   * @param n_legs Maximum number of legs in Contest task
   * @param finish_alt_diff Maximum height loss from start to finish (m)
   */
  ContestDijkstra(const Trace &_trace, 
                  const unsigned &_handicap,
                  const unsigned n_legs,
                  const unsigned finish_alt_diff = 1000);

  bool score(ContestResult &result);

  virtual void copy_solution(ContestTraceVector &vec) const;

protected:
  virtual fixed calc_distance() const;
  virtual fixed calc_score() const;
  virtual fixed calc_time() const;

public:
  /**
   * Reset the optimiser as if never flown
   */
  virtual void reset();

  /**
   * Update the solver.  The solver is incremental, so this method can
   * be safely called every time step.
   *
   * @return True if solver completed in this call
   */
  virtual bool solve(bool exhaustive);

protected:
  /** Number of points in current trace set */
  unsigned n_points;

  /** Max number of points in trace set */
  static const unsigned max_contest_trace;

  /** Update working trace from master --- never to be done during a solution! */
  virtual void update_trace();

  void clear_trace();

  TracePointVector trace; // working trace for solver

  /**
   * Determine if a trace point can be added to the search list
   *
   * @param candidate The index to the candidate
   * @return True if candidate is valid
   */
  bool admit_candidate(const ScanTaskPoint &candidate) const;

  const TracePoint &get_point(const ScanTaskPoint &sp) const;


  virtual void add_edges(const ScanTaskPoint &curNode);

  /**
   * Set weightings of each leg.  Default is constant weighting.
   */
  virtual void set_weightings();

  /** Weightings applied to each leg distance */
  unsigned m_weightings[MAX_STAGES];

  /**
   * Retrieve weighting of specified leg
   * @param index Index of leg
   * @return Weighting of leg
   */
  unsigned get_weighting(const unsigned index) const;

  /**
   * Perform actions required at start of new search
   */
  virtual void start_search();

  virtual bool save_solution();

  ContestTraceVector best_solution;

private:
  bool solution_found;
  bool trace_dirty;
  virtual void add_start_edges();
  TracePoint last_point;
  bool master_is_updated();

public: // instrumentation
  static unsigned long count_olc_solve;
  static unsigned long count_olc_trace;
  static unsigned count_olc_size;
};

#endif
