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

#include "Engine/Trace/Trace.hpp"
#include "Contest/ContestManager.hpp"
#include "OS/Args.hpp"
#include "Computer/CirclingComputer.hpp"
#include "DebugReplay.hpp"
#include "Util/Macros.hpp"
#include "IO/StdioOutputStream.hxx"
#include "Formatter/TimeFormatter.hpp"
#include "JSON/Writer.hpp"
#include "JSON/GeoWriter.hpp"
#include "FlightPhaseDetector.hpp"
#include "FlightPhaseJSON.hpp"
#include "Computer/Settings.hpp"
#include "Util/StringCompare.hxx"

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

  if (state.release_time >= 0 && !result.release_time.IsPlausible()) {
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

    if (!released && replay.Calculated().flight.release_time >= 0) {
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
             Trace &full_trace, Trace &triangle_trace, Trace &sprint_trace)
{
  ContestManager manager(contest, full_trace, triangle_trace, sprint_trace);
  manager.SolveExhaustive();
  return manager.GetStats();
}

static void
WriteEventAttributes(BufferedOutputStream &writer,
                     const BrokenDateTime &time, const GeoPoint &location)
{
  JSON::ObjectWriter object(writer);

  if (time.IsPlausible()) {
    NarrowString<64> buffer;
    FormatISO8601(buffer.buffer(), time);
    object.WriteElement("time", JSON::WriteString, buffer);
  }

  if (location.IsValid())
    JSON::WriteGeoPointAttributes(object, location);
}

static void
WriteEvent(JSON::ObjectWriter &object, const char *name,
           const BrokenDateTime &time, const GeoPoint &location)
{
  if (time.IsPlausible() || location.IsValid())
    object.WriteElement(name, WriteEventAttributes, time, location);
}

static void
WriteEvents(BufferedOutputStream &writer, const Result &result)
{
  JSON::ObjectWriter object(writer);

  WriteEvent(object, "takeoff", result.takeoff_time, result.takeoff_location);
  WriteEvent(object, "release", result.release_time, result.release_location);
  WriteEvent(object, "landing", result.landing_time, result.landing_location);
}

static void
WriteResult(JSON::ObjectWriter &root, const Result &result)
{
  root.WriteElement("events", WriteEvents, result);
}

static void
WritePoint(BufferedOutputStream &writer, const ContestTracePoint &point,
           const ContestTracePoint *previous)
{
  JSON::ObjectWriter object(writer);

  object.WriteElement("time", JSON::WriteLong, (long)point.GetTime());
  JSON::WriteGeoPointAttributes(object, point.GetLocation());

  if (previous != NULL) {
    auto distance = point.DistanceTo(previous->GetLocation());
    object.WriteElement("distance", JSON::WriteUnsigned, uround(distance));

    unsigned duration =
      std::max((int)point.GetTime() - (int)previous->GetTime(), 0);
    object.WriteElement("duration", JSON::WriteUnsigned, duration);

    if (duration > 0) {
      auto speed = distance / duration;
      object.WriteElement("speed", JSON::WriteDouble, speed);
    }
  }
}

static void
WriteTrace(BufferedOutputStream &writer, const ContestTraceVector &trace)
{
  JSON::ArrayWriter array(writer);

  const ContestTracePoint *previous = NULL;
  for (auto i = trace.begin(), end = trace.end(); i != end; ++i) {
    array.WriteElement(WritePoint, *i, previous);
    previous = &*i;
  }
}

static void
WriteContest(BufferedOutputStream &writer,
             const ContestResult &result, const ContestTraceVector &trace)
{
  JSON::ObjectWriter object(writer);

  object.WriteElement("score", JSON::WriteDouble, result.score);
  object.WriteElement("distance", JSON::WriteDouble, result.distance);
  object.WriteElement("duration", JSON::WriteUnsigned, (unsigned)result.time);
  object.WriteElement("speed", JSON::WriteDouble, result.GetSpeed());

  object.WriteElement("turnpoints", WriteTrace, trace);
}

static void
WriteOLCPlus(BufferedOutputStream &writer, const ContestStatistics &stats)
{
  JSON::ObjectWriter object(writer);

  object.WriteElement("classic", WriteContest,
                      stats.result[0], stats.solution[0]);
  object.WriteElement("triangle", WriteContest,
                      stats.result[1], stats.solution[1]);
  object.WriteElement("plus", WriteContest,
                      stats.result[2], stats.solution[2]);
}

static void
WriteDMSt(BufferedOutputStream &writer, const ContestStatistics &stats)
{
  JSON::ObjectWriter object(writer);

  object.WriteElement("quadrilateral", WriteContest,
                      stats.result[0], stats.solution[0]);
}

static void
WriteContests(BufferedOutputStream &writer, const ContestStatistics &olc_plus,
              const ContestStatistics &dmst)
{
  JSON::ObjectWriter object(writer);

  object.WriteElement("olc_plus", WriteOLCPlus, olc_plus);
  object.WriteElement("dmst", WriteDMSt, dmst);
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

  static Trace full_trace(0, Trace::null_time, full_max_points);
  static Trace triangle_trace(0, Trace::null_time, triangle_max_points);
  static Trace sprint_trace(0, 9000, sprint_max_points);

  Result result;
  Run(*replay, result, full_trace, triangle_trace, sprint_trace);
  delete replay;

  const ContestStatistics olc_plus = SolveContest(Contest::OLC_PLUS, full_trace, triangle_trace, sprint_trace);
  const ContestStatistics dmst = SolveContest(Contest::DMST, full_trace, triangle_trace, sprint_trace);

  StdioOutputStream os(stdout);
  BufferedOutputStream writer(os);

  {
    JSON::ObjectWriter root(writer);

    WriteResult(root, result);
    root.WriteElement("phases", WritePhaseList,
                      flight_phase_detector.GetPhases());
    root.WriteElement("performance", WritePerformanceStats,
                      flight_phase_detector.GetTotals());
    root.WriteElement("contests", WriteContests, olc_plus, dmst);
  }

  writer.Flush();
}
