/* Copyright_License {

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

#ifndef ONLINE_CONTEST_HPP
#define ONLINE_CONTEST_HPP

#include "Settings.hpp"
#include "Solvers/OLCSprint.hpp"
#include "Solvers/OLCFAI.hpp"
#include "Solvers/OLCClassic.hpp"
#include "Solvers/OLCLeague.hpp"
#include "Solvers/OLCPlus.hpp"
#include "Solvers/DMStQuad.hpp"
#include "Solvers/XContestFree.hpp"
#include "Solvers/XContestTriangle.hpp"
#include "Solvers/OLCSISAT.hpp"
#include "Solvers/NetCoupe.hpp"
#include "ContestStatistics.hpp"

class Trace;

/**
 * Special task holder for Online Contest calculations
 */
class ContestManager
{
  friend class PrintHelper;

  Contest contest;

  ContestStatistics stats;

  OLCSprint olc_sprint;
  OLCFAI olc_fai;
  OLCClassic olc_classic;
  OLCLeague olc_league;
  OLCPlus olc_plus;
  DMStQuad dmst_quad;
  XContestFree xcontest_free;
  XContestTriangle xcontest_triangle;
  XContestFree dhv_xc_free;
  XContestTriangle dhv_xc_triangle;
  OLCSISAT sis_at;
  NetCoupe net_coupe;

public:
  /**
   * Base constructor.
   *
   * @param _contest Contest that shall be used
   * @param trace_full Trace object reference
   * containing full flight history for scanning
   * @param trace_full Trace object reference
   * containing full flight history for triangle scanning
   * (should contain more than 1024 points).
   * @param trace_sprint Trace object reference
   * containing 2.5 hour flight history for scanning
   * @param predict_triangle assume the the pilot will close the
   * triangle?
   */
  ContestManager(const Contest _contest,
                 const Trace &trace_full,
                 const Trace &trace_triangle,
                 const Trace &trace_sprint,
                 bool predict_triangle=false);

  void SetIncremental(bool incremental);

  /**
   * @see ContestDijkstra::SetPredicted()
   */
  void SetPredicted(const TracePoint &predicted);

  void SetContest(Contest _contest) {
    contest = _contest;
  }

  void SetHandicap(unsigned handicap);

  /**
   * Update internal states (non-essential) for housework,
   * or where functions are slow and would cause loss to real-time performance.
   *
   * @param exhaustive true to find the final solution, false stops
   * after a number of iterations (incremental search)
   * @return True if internal state changed
   */
  bool UpdateIdle(bool exhaustive = false);

  bool SolveExhaustive() {
    return UpdateIdle(true);
  }

  /**
   * Solve exhaustive with custom computational limits for the triangle solver.
   */
  bool SolveExhaustive(unsigned max_iterations, unsigned max_tree_size) {
    olc_fai.SetMaxIterations(max_iterations);
    olc_fai.SetMaxTreeSize(max_tree_size);
    dhv_xc_triangle.SetMaxIterations(max_iterations);
    dhv_xc_triangle.SetMaxTreeSize(max_tree_size);

    return SolveExhaustive();
  }

  /**
   * Reset the task (as if never flown)
   */
  void Reset();

  const ContestStatistics &GetStats() const {
    return stats;
  }
};

#endif
