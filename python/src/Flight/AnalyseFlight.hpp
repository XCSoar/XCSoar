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

#ifndef PYTHON_ANALYSEFLIGHT_HPP
#define PYTHON_ANALYSEFLIGHT_HPP

#include "FlightPhaseDetector.hpp"
#include "Contest/Settings.hpp"
#include "Geo/SpeedVector.hpp"
#include "Time/BrokenDateTime.hpp"

#include <list>

class DebugReplay;
class Trace;
struct ContestStatistics;
struct ComputerSettings;

struct WindListItem {
  // Date and time of measurement
  BrokenDateTime datetime;

  // Altitude of measurement
  double altitude;

  // Speed vector of measurement
  SpeedVector wind;

  WindListItem(BrokenDateTime _datetime, double _altitude, SpeedVector _wind)
    :datetime(_datetime), altitude(_altitude), wind(_wind) {};
};

typedef std::list<WindListItem> WindList;

void
Run(DebugReplay &replay, FlightPhaseDetector &flight_phase_detector,
    WindList &wind_list,
    const BrokenDateTime &takeoff_time,
    const BrokenDateTime &scoring_start_time,
    const BrokenDateTime &scoring_end_time,
    const BrokenDateTime &landing_time,
    Trace &full_trace, Trace &triangle_trace, Trace &sprint_trace,
    ComputerSettings &computer_settings);

ContestStatistics
SolveContest(Contest contest,
             Trace &full_trace, Trace &triangle_trace, Trace &sprint_trace,
             const unsigned max_iterations, const unsigned max_tree_size);

void AnalyseFlight(DebugReplay &replay,
             const BrokenDateTime &takeoff_time,
             const BrokenDateTime &scoring_start_time,
             const BrokenDateTime &scoring_end_time,
             const BrokenDateTime &landing_time,
             ContestStatistics &olc_plus,
             ContestStatistics &dmst,
             PhaseList &phase_list,
             PhaseTotals &phase_totals,
             WindList &wind_list,
             ComputerSettings &computer_settings,
             const unsigned full_points = 512,
             const unsigned triangle_points = 1024,
             const unsigned sprint_points = 96,
             const unsigned max_iterations = 20e6,
             const unsigned max_tree_size = 5e6);

#endif /* PYTHON_ANALYSEFLIGHT_HPP */
