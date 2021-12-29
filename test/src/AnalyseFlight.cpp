/* Copyright_License {

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

#include "Engine/Trace/Trace.hpp"
#include "Contest/ContestManager.hpp"
#include "system/Args.hpp"
#include "Computer/CirclingComputer.hpp"
#include "DebugReplay.hpp"
#include "util/Macros.hpp"
#include "io/StdioOutputStream.hxx"
#include "Formatter/TimeFormatter.hpp"
#include "json/Geo.hpp"
#include "json/Serialize.hxx"
#include "FlightPhaseDetector.hpp"
#include "FlightPhaseJSON.hpp"
#include "Computer/Settings.hpp"
#include "util/StringCompare.hxx"

using namespace std::chrono;

struct Result {
  BrokenDateTime takeoff_time, release_time, landing_time;
  GeoPoint takeoff_location, release_location, landing_location;

  Result() {
    takeoff_time.Clear();
    landing_time.Clear();
    release_time.Clear();

    takeoff_location.SetInvalid();
    landing_location.SetInvalid();
    release_location.SetInvalid();
  }
};


static CirclingComputer circling_computer;
static FlightPhaseDetector flight_phase_detector;

static void
Update(const MoreData &basic, const FlyingState &state,
       Result &result)
{
  if (!basic.time_available || !basic.date_time_utc.IsDatePlausible())
    return;

  if (state.flying && !result.takeoff_time.IsPlausible()) {
    result.takeoff_time = basic.GetDateTimeAt(state.takeoff_time);
    result.takeoff_location = state.takeoff_location;
  }

  if (!state.flying && result.takeoff_time.IsPlausible() &&
      !result.landing_time.IsPlausible()) {
    result.landing_time = basic.GetDateTimeAt(state.landing_time);
    result.landing_location = state.landing_location;
  }

  if (state.release_time.IsDefined() && !result.release_time.IsPlausible()) {
    result.release_time = basic.GetDateTimeAt(state.release_time);
    result.release_location = state.release_location;
  }
}

static void
Update(const MoreData &basic, const DerivedInfo &calculated,
       Result &result)
{
  Update(basic, calculated.flight, result);
}

static void
ComputeCircling(DebugReplay &replay, const CirclingSettings &circling_settings)
{
  circling_computer.TurnRate(replay.SetCalculated(),
                             replay.Basic(),
                             replay.Calculated().flight);
  circling_computer.Turning(replay.SetCalculated(),
                            replay.Basic(),
                            replay.Calculated().flight,
                            circling_settings);
}

static void
Finish(const MoreData &basic, const DerivedInfo &calculated,
       Result &result)
{
  if (!basic.time_available || !basic.date_time_utc.IsDatePlausible())
    return;

  if (result.takeoff_time.IsPlausible() && !result.landing_time.IsPlausible()) {
    result.landing_time = basic.date_time_utc;

    if (basic.location_available)
      result.landing_location = basic.location;
  }
}

static void
Run(DebugReplay &replay, Result &result,
    Trace &full_trace, Trace &triangle_trace, Trace &sprint_trace)
{
  CirclingSettings circling_settings;
  circling_settings.SetDefaults();

  bool released = false;

  GeoPoint last_location = GeoPoint::Invalid();
  constexpr Angle max_longitude_change = Angle::Degrees(30);
  constexpr Angle max_latitude_change = Angle::Degrees(1);

  while (replay.Next()) {
    ComputeCircling(replay, circling_settings);

    const MoreData &basic = replay.Basic();

    Update(basic, replay.Calculated(), result);
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

    if (!released && replay.Calculated().flight.release_time.IsDefined()) {
      released = true;

      full_trace.EraseEarlierThan(replay.Calculated().flight.release_time);
      triangle_trace.EraseEarlierThan(replay.Calculated().flight.release_time);
      sprint_trace.EraseEarlierThan(replay.Calculated().flight.release_time);
    }

    if (released && !replay.Calculated().flight.flying)
      /* the aircraft has landed, stop here */
      /* TODO: at some point, we might want to emit the analysis of
         all flights in this IGC file */
      break;

    const TracePoint point(basic);
    full_trace.push_back(point);
    triangle_trace.push_back(point);
    sprint_trace.push_back(point);
  }

  Update(replay.Basic(), replay.Calculated(), result);
  Finish(replay.Basic(), replay.Calculated(), result);
  flight_phase_detector.Finish();
}

gcc_pure
static ContestStatistics
SolveContest(Contest contest,
             Trace &full_trace, Trace &triangle_trace,
             Trace &sprint_trace) noexcept
{
  ContestManager manager(contest, full_trace, triangle_trace, sprint_trace);
  manager.SolveExhaustive();
  return manager.GetStats();
}

static boost::json::object
WriteEventAttributes(const BrokenDateTime &time,
                     const GeoPoint &location) noexcept
{
  boost::json::object o;
  if (location.IsValid())
    o = boost::json::value_from(location).as_object();

  if (time.IsPlausible()) {
    NarrowString<64> buffer;
    FormatISO8601(buffer.buffer(), time);
    o.emplace("time", buffer.c_str());
  }

  return o;
}

static void
WriteEvent(boost::json::object &parent, const char *name,
           const BrokenDateTime &time, const GeoPoint &location) noexcept
{
  if (time.IsPlausible() || location.IsValid())
    parent.emplace(name, WriteEventAttributes(time, location));
}

static boost::json::object
WriteEvents(const Result &result) noexcept
{
  boost::json::object object;

  WriteEvent(object, "takeoff", result.takeoff_time, result.takeoff_location);
  WriteEvent(object, "release", result.release_time, result.release_location);
  WriteEvent(object, "landing", result.landing_time, result.landing_location);

  return object;
}

static void
WriteResult(boost::json::object &root, const Result &result) noexcept
{
  root.emplace("events", WriteEvents(result));
}

static boost::json::object
WritePoint(const ContestTracePoint &point,
           const ContestTracePoint *previous) noexcept
{
  boost::json::object object =
    boost::json::value_from(point.GetLocation()).as_object();

  object.emplace("time", (long)point.GetTime().count());

  if (previous != NULL) {
    auto distance = point.DistanceTo(previous->GetLocation());
    object.emplace("distance", uround(distance));

    const auto duration = std::max(point.GetTime() - previous->GetTime(),
                                   std::chrono::duration<unsigned>{});
    object.emplace("duration", (int)duration.count());

    if (duration.count() > 0) {
      const double speed = distance / duration.count();
      object.emplace("speed", speed);
    }
  }

  return object;
}

static boost::json::array
WriteTrace(const ContestTraceVector &trace) noexcept
{
  boost::json::array array;

  const ContestTracePoint *previous = NULL;
  for (auto i = trace.begin(), end = trace.end(); i != end; ++i) {
    array.emplace_back(WritePoint(*i, previous));
    previous = &*i;
  }

  return array;
}

static boost::json::object
WriteContest(const ContestResult &result,
             const ContestTraceVector &trace) noexcept
{
  boost::json::object object;

  object.emplace("score", result.score);
  object.emplace("distance", result.distance);
  object.emplace("duration", (unsigned)result.time.count());
  object.emplace("speed", result.GetSpeed());

  object.emplace("turnpoints", WriteTrace(trace));

  return object;
}

static boost::json::object
WriteOLCPlus(const ContestStatistics &stats) noexcept
{
  boost::json::object object;

  object.emplace("classic", WriteContest(stats.result[0], stats.solution[0]));
  object.emplace("triangle", WriteContest(stats.result[1], stats.solution[1]));
  object.emplace("plus", WriteContest(stats.result[2], stats.solution[2]));

  return object;
}

static boost::json::object
WriteDMSt(const ContestStatistics &stats) noexcept
{
  boost::json::object object;

  object.emplace("quadrilateral",
                 WriteContest(stats.result[0], stats.solution[0]));

  return object;
}

static boost::json::object
WriteContests(const ContestStatistics &olc_plus,
              const ContestStatistics &dmst) noexcept
{
  boost::json::object object;

  object.emplace("olc_plus", WriteOLCPlus(olc_plus));
  object.emplace("dmst", WriteDMSt(dmst));

  return object;
}

int main(int argc, char **argv)
{
  unsigned full_max_points = 512,
           triangle_max_points = 1024,
           sprint_max_points = 64;

  Args args(argc, argv,
            "[options] DRIVER FILE\n"
            "Options:\n"
            "  --full-points=512        Maximum number of full trace points (default = 512)\n"
            "  --triangle-points=1024   Maximum number of triangle trace points (default = 1024)\n"
            "  --sprint-points=64       Maximum number of sprint trace points (default = 64)");

  const char *arg;
  while ((arg = args.PeekNext()) != nullptr && *arg == '-') {
    args.Skip();

    const char *value;
    if ((value = StringAfterPrefix(arg, "--full-points=")) != nullptr) {
      unsigned _points = strtol(value, NULL, 10);
      if (_points > 0)
        full_max_points = _points;
      else {
        fputs("The start parameter could not be parsed correctly.\n", stderr);
        args.UsageError();
      }

    } else if ((value = StringAfterPrefix(arg, "--triangle-points=")) != nullptr) {
      unsigned _points = strtol(value, NULL, 10);
      if (_points > 0)
        triangle_max_points = _points;
      else {
        fputs("The start parameter could not be parsed correctly.\n", stderr);
        args.UsageError();
      }

    } else if ((value = StringAfterPrefix(arg, "--sprint-points=")) != nullptr) {
      unsigned _points = strtol(value, NULL, 10);
      if (_points > 0)
        sprint_max_points = _points;
      else {
        fputs("The start parameter could not be parsed correctly.\n", stderr);
        args.UsageError();
      }

    } else {
      args.UsageError();
    }
  }

  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  static Trace full_trace({}, Trace::null_time, full_max_points);
  static Trace triangle_trace({}, Trace::null_time, triangle_max_points);
  static Trace sprint_trace({}, minutes{150}, sprint_max_points);

  Result result;
  Run(*replay, result, full_trace, triangle_trace, sprint_trace);
  delete replay;

  const ContestStatistics olc_plus = SolveContest(Contest::OLC_PLUS, full_trace, triangle_trace, sprint_trace);
  const ContestStatistics dmst = SolveContest(Contest::DMST, full_trace, triangle_trace, sprint_trace);

  StdioOutputStream os(stdout);

  {
    boost::json::object root;

    WriteResult(root, result);
    root.emplace("phases", WritePhaseList(flight_phase_detector.GetPhases()));
    root.emplace("performance",
                 WritePerformanceStats(flight_phase_detector.GetTotals()));
    root.emplace("contests", WriteContests(olc_plus, dmst));

    Json::Serialize(os, root);
  }

  return EXIT_SUCCESS;
}
