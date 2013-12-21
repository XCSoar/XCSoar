/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "AnalyseFlight.hpp"
#include "DebugReplay.hpp"
#include "Engine/Trace/Trace.hpp"
#include "Contest/ContestManager.hpp"
#include "Math/Angle.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Computer/CirclingComputer.hpp"
#include "Computer/Settings.hpp"
#include "FlightPhaseDetector.hpp"

void
Run(DebugReplay &replay, FlightPhaseDetector &flight_phase_detector,
    const BrokenDateTime &takeoff_time,
    const BrokenDateTime &release_time,
    const BrokenDateTime &landing_time,
    Trace &full_trace, Trace &triangle_trace, Trace &sprint_trace)
{
  GeoPoint last_location = GeoPoint::Invalid();
  constexpr Angle max_longitude_change = Angle::Degrees(30);
  constexpr Angle max_latitude_change = Angle::Degrees(1);

  CirclingSettings circling_settings;
  circling_settings.SetDefaults();
  CirclingComputer circling_computer;

  const int64_t takeoff_unix = takeoff_time.ToUnixTimeUTC();
  const int64_t release_unix = release_time.ToUnixTimeUTC();
  const int64_t landing_unix = landing_time.ToUnixTimeUTC();

  while (replay.Next()) {
    const MoreData &basic = replay.Basic();
    const int64_t date_time_utc = basic.date_time_utc.ToUnixTimeUTC();

    if (date_time_utc < takeoff_unix)
      continue;

    if (date_time_utc > landing_unix)
      break;

    circling_computer.TurnRate(replay.SetCalculated(),
                               replay.Basic(),
                               replay.Calculated().flight);
    circling_computer.Turning(replay.SetCalculated(),
                              replay.Basic(),
                              replay.Calculated().flight,
                              circling_settings);

    flight_phase_detector.Update(replay.Basic(), replay.Calculated());

    if (!basic.time_available || !basic.location_available ||
        !basic.NavAltitudeAvailable())
      continue;

    if (last_location.IsValid() &&
        ((last_location.latitude - basic.location.latitude).Absolute() > max_latitude_change ||
         (last_location.longitude - basic.location.longitude).Absolute() > max_longitude_change))
      /* there was an implausible warp, which is usually triggered by
         an invalid point declared "valid" by a bugged logger; if that
         happens, we stop the analysis, because the IGC file is
         obviously broken */
      break;

    last_location = basic.location;

    if (date_time_utc >= release_unix) {
      const TracePoint point(basic);
      full_trace.push_back(point);
      triangle_trace.push_back(point);
      sprint_trace.push_back(point);
    }
  }

  flight_phase_detector.Finish();
}

ContestStatistics
SolveContest(Contest contest,
             Trace &full_trace, Trace &triangle_trace, Trace &sprint_trace)
{
  ContestManager manager(contest, full_trace, triangle_trace, sprint_trace);
  manager.SolveExhaustive();
  return manager.GetStats();
}

void AnalyseFlight(DebugReplay &replay,
             const BrokenDateTime &takeoff_time,
             const BrokenDateTime &release_time,
             const BrokenDateTime &landing_time,
             ContestStatistics &olc_plus,
             ContestStatistics &dmst,
             PhaseList &phase_list,
             PhaseTotals &phase_totals,
             const unsigned full_points,
             const unsigned triangle_points,
             const unsigned sprint_points)
{
  Trace full_trace(0, Trace::null_time, full_points);
  Trace triangle_trace(0, Trace::null_time, triangle_points);
  Trace sprint_trace(0, 9000, sprint_points);
  FlightPhaseDetector flight_phase_detector;

  Run(replay, flight_phase_detector,
      takeoff_time, release_time, landing_time,
      full_trace, triangle_trace, sprint_trace);

  olc_plus = SolveContest(Contest::OLC_PLUS, full_trace, triangle_trace, sprint_trace);
  dmst = SolveContest(Contest::DMST, full_trace, triangle_trace, sprint_trace);

  phase_list = flight_phase_detector.GetPhases();
  phase_totals = flight_phase_detector.GetTotals();
}
