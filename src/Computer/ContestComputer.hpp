// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Contest/ContestManager.hpp"

struct ContestSettings;
struct ContestStatistics;
class Trace;

class ContestComputer {
  ContestManager contest_manager;

public:
  ContestComputer(const Trace &trace_full,
                  const Trace &trace_triangle,
                  const Trace &trace_sprint);

  void SetIncremental(bool incremental) {
    contest_manager.SetIncremental(incremental);
  }

  void Reset() {
    contest_manager.Reset();
  }

  /**
   * @see ContestDijkstra::SetPredicted()
   */
  void SetPredicted(const TracePoint &predicted) {
    contest_manager.SetPredicted(predicted);
  }

  void Solve(const ContestSettings &settings_computer,
             ContestStatistics &contest_stats);

  bool SolveExhaustive(const ContestSettings &settings_computer,
                       ContestStatistics &contest_stats);
};
