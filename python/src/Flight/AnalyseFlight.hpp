// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FlightPhaseDetector.hpp"
#include "Contest/Settings.hpp"
#include "Geo/SpeedVector.hpp"
#include "time/BrokenDateTime.hpp"

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
