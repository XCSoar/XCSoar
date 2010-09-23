/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "NavDijkstra.hpp"
#include "Navigation/TracePoint.hpp"
#include "Math/fixed.hpp"

class OnlineContest;

/**
 * Abstract class for OLC path searches
 *
 */
class OLCDijkstra:
  public NavDijkstra<TracePoint>
{
public:
  /**
   * Constructor
   *
   * @param _olc The OLC task to solve for
   * @param n_legs Maximum number of legs in OLC task
   * @param finish_alt_diff Maximum height loss from start to finish (m)
   * @param full_trace Whether this OLC algorithm requires the full history or just the first 2.5 hours
   */
  OLCDijkstra(OnlineContest& _olc, const unsigned n_legs,
              const unsigned finish_alt_diff = 3000,
              const bool full_trace = true);

  /**
   * Calculate the scored value of the OLC path
   *
   * @param the_distance output distance (m) of scored path
   * @param the_speed output speed (m/s) of scored path
   * @param the_time output time (s) of scored path
   *
   * @return Score (interpretation depends on OLC type)
   */
  fixed score(fixed &the_distance, fixed &the_speed, fixed &the_time);

  /**
   * Copy the best OLC path solution
   *
   * @param vec output vector
   */
  void copy_solution(TracePointVector &vec);

  /**
   * Calculate distance of best path
   *
   * @return Distance (m)
   */
  fixed calc_distance() const;

  /**
   * Calculate elapsed time of best path
   *
   * @return Distance (m)
   */
  fixed calc_time() const;

  /**
   * Reset the optimiser as if never flown
   *
   */
  virtual void reset();

  /**
   * Update the solver.  The solver is incremental, so this method can
   * be safely called every time step.
   *
   * @return True if solver completed in this call
   */
  bool solve();

protected:
  const TracePoint &get_point(const ScanTaskPoint &sp) const;

  virtual void add_edges(DijkstraTaskPoint &dijkstra,
                         const ScanTaskPoint &curNode);

  virtual bool finish_satisfied(const ScanTaskPoint &sp) const;

  /**
   * Set weightings of each leg.  Default is constant weighting.
   */
  virtual void set_weightings();

  /**
   * Determine if a trace point can be added to the search list
   *
   * @param candidate The index to the candidate
   * @return True if candidate is valid
   */
  virtual bool admit_candidate(const ScanTaskPoint &candidate) const;

  /** Weightings applied to each leg distance */
  unsigned m_weightings[MAX_STAGES];

  /** Dijkstra search algorithm */
  DijkstraTaskPoint m_dijkstra;

  /** Number of points in current trace set */
  unsigned n_points;

  /**
   * Retrieve weighting of specified leg
   * @param index Index of leg
   * @return Weighting of leg
   */
  unsigned get_weighting(const unsigned index) const;

private:
  /**
   * Iterate solver
   * @return True if solver returned
   */
  bool solve_inner();

  OnlineContest& olc;
  const unsigned m_finish_alt_diff;

  void save_solution();

  bool solution_found;
  fixed best_distance;
  fixed best_speed;
  fixed best_time;

  TracePoint best_solution[MAX_STAGES];

  virtual void add_start_edges();

  const bool m_full_trace;
};

#endif
