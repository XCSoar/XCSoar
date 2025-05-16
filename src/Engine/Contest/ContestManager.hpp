// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
#include "Solvers/WeglideFree.hpp"
#include "Solvers/WeglideDistance.hpp"
#include "Solvers/WeglideFAI.hpp"
#include "Solvers/WeglideOR.hpp"
#include "Solvers/Charron.hpp"
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
  WeglideFree weglide_free;
  WeglideDistance weglide_distance;
  WeglideFAI weglide_fai;
  WeglideOR weglide_or;
  Charron charron_small;
  Charron charron_large;

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
                 bool predict_triangle=false) noexcept;

  void SetIncremental(bool incremental) noexcept;

  /**
   * @see ContestDijkstra::SetPredicted()
   */
  void SetPredicted(const TracePoint &predicted) noexcept;

  void SetContest(Contest _contest) noexcept {
    contest = _contest;
  }

  void SetHandicap(unsigned handicap) noexcept;

  /**
   * Update internal states (non-essential) for housework,
   * or where functions are slow and would cause loss to real-time performance.
   *
   * @param exhaustive true to find the final solution, false stops
   * after a number of iterations (incremental search)
   * @return True if internal state changed
   */
  bool UpdateIdle(bool exhaustive = false) noexcept;

  bool SolveExhaustive() noexcept {
    return UpdateIdle(true);
  }

  /**
   * Solve exhaustive with custom computational limits for the triangle solver.
   */
  bool SolveExhaustive(unsigned max_iterations,
                       unsigned max_tree_size) noexcept {
    olc_fai.SetMaxIterations(max_iterations);
    olc_fai.SetMaxTreeSize(max_tree_size);
    dhv_xc_triangle.SetMaxIterations(max_iterations);
    dhv_xc_triangle.SetMaxTreeSize(max_tree_size);
    weglide_fai.SetMaxIterations(max_iterations);
    weglide_fai.SetMaxTreeSize(max_tree_size);

    return SolveExhaustive();
  }

  /**
   * Reset the task (as if never flown)
   */
  void Reset() noexcept;

  const ContestStatistics &GetStats() const noexcept {
    return stats;
  }
};
