/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_CONTEST_COMPUTER_HPP
#define XCSOAR_CONTEST_COMPUTER_HPP

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

#endif
