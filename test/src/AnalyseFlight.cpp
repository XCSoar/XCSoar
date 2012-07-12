/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Engine/Navigation/Aircraft.hpp"
#include "OS/Args.hpp"
#include "DebugReplay.hpp"
#include "NMEA/Aircraft.hpp"
#include "Util/Macros.hpp"
#include "IO/TextWriter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "JSON/Writer.hpp"
#include "JSON/GeoWriter.hpp"

struct Result {
  BrokenDateTime takeoff_time, landing_time;
  GeoPoint takeoff_location, landing_location;

  Result() {
    takeoff_time.Clear();
    landing_time.Clear();

    takeoff_location.SetInvalid();
    landing_location.SetInvalid();
  }
};

static Trace full_trace(60, Trace::null_time, 256);
static Trace sprint_trace(0, 9000, 64);

static BrokenDateTime
BreakTime(const NMEAInfo &basic, fixed time)
{
  assert(basic.time_available);
  assert(basic.date_available);

  BrokenDateTime broken = basic.date_time_utc;
  (BrokenTime &)broken = BrokenTime::FromSecondOfDayChecked((unsigned)time);

  // XXX: what if "time" refers to yesterday/tomorrow? (midnight rollover)

  return broken;
}

static void
Update(const MoreData &basic, const DerivedInfo &calculated,
       Result &result)
{
  if (!basic.time_available || !basic.date_available)
    return;

  if (calculated.flight.flying && !result.takeoff_time.Plausible()) {
    result.takeoff_time = BreakTime(basic, calculated.flight.takeoff_time);
    result.takeoff_location = calculated.flight.takeoff_location;
  }

  if (!calculated.flight.flying && result.takeoff_time.Plausible() &&
      !result.landing_time.Plausible()) {
    result.landing_time = basic.date_time_utc;

    if (basic.location_available)
      result.landing_location = basic.location;
  }
}

static void
Finish(const MoreData &basic, const DerivedInfo &calculated,
       Result &result)
{
  if (!basic.time_available || !basic.date_available)
    return;

  if (result.takeoff_time.Plausible() && !result.landing_time.Plausible()) {
    result.landing_time = basic.date_time_utc;

    if (basic.location_available)
      result.landing_location = basic.location;
  }
}

static int
Run(DebugReplay &replay, ContestManager &contest, Result &result)
{
  for (int i = 1; replay.Next(); i++) {
    Update(replay.Basic(), replay.Calculated(), result);

    const AircraftState state =
      ToAircraftState(replay.Basic(), replay.Calculated());
    full_trace.push_back(state);
    sprint_trace.push_back(state);
  }

  contest.SolveExhaustive();

  Finish(replay.Basic(), replay.Calculated(), result);

  return 0;
}

static void
WriteTimes(TextWriter &writer, const Result &result)
{
  JSON::ObjectWriter object(writer);
  NarrowString<64> buffer;

  if (result.takeoff_time.Plausible()) {
    FormatISO8601(buffer.buffer(), result.takeoff_time);
    object.WriteElement("takeoff", JSON::WriteString, buffer);
  }

  if (result.landing_time.Plausible()) {
    FormatISO8601(buffer.buffer(), result.landing_time);
    object.WriteElement("landing", JSON::WriteString, buffer);
  }
}

static void
WriteLocations(TextWriter &writer, const Result &result)
{
  JSON::ObjectWriter object(writer);

  if (result.takeoff_location.IsValid())
    object.WriteElement("takeoff", JSON::WriteGeoPoint,
                        result.takeoff_location);

  if (result.landing_location.IsValid())
    object.WriteElement("landing", JSON::WriteGeoPoint,
                        result.landing_location);
}

static void
WriteResult(JSON::ObjectWriter &root, const Result &result)
{
  root.WriteElement("times", WriteTimes, result);
  root.WriteElement("locations", WriteLocations, result);
}

static void
WritePoint(TextWriter &writer, const TracePoint &point,
           const TracePoint *previous)
{
  JSON::ObjectWriter object(writer);

  object.WriteElement("time", JSON::WriteLong, (long)point.GetTime());
  JSON::WriteGeoPointAttributes(object, point.GetLocation());

  if (previous != NULL) {
    fixed distance = point.DistanceTo(previous->GetLocation());
    object.WriteElement("distance", JSON::WriteUnsigned, uround(distance));

    unsigned duration =
      std::max((int)point.GetTime() - (int)previous->GetTime(), 0);
    object.WriteElement("duration", JSON::WriteUnsigned, duration);

    if (duration > 0) {
      fixed speed = distance / duration;
      object.WriteElement("speed", JSON::WriteFixed, speed);
    }
  }
}

static void
WriteTrace(TextWriter &writer, const ContestTraceVector &trace)
{
  JSON::ArrayWriter array(writer);

  const TracePoint *previous = NULL;
  for (auto i = trace.begin(), end = trace.end(); i != end; ++i) {
    array.WriteElement(WritePoint, *i, previous);
    previous = &*i;
  }
}

static void
WriteContest(TextWriter &writer,
             const ContestResult &result, const ContestTraceVector &trace)
{
  JSON::ObjectWriter object(writer);

  object.WriteElement("score", JSON::WriteFixed, result.score);
  object.WriteElement("distance", JSON::WriteFixed, result.distance);
  object.WriteElement("duration", JSON::WriteUnsigned, (unsigned)result.time);
  object.WriteElement("speed", JSON::WriteFixed, result.GetSpeed());

  object.WriteElement("turnpoints", WriteTrace, trace);
}

static void
WriteOLCPlus(TextWriter &writer, const ContestStatistics &stats)
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
WriteContests(TextWriter &writer, const ContestStatistics &olc_plus)
{
  JSON::ObjectWriter object(writer);

  object.WriteElement("olc_plus", WriteOLCPlus, olc_plus);
}

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  ContestManager olc_plus(OLC_Plus, full_trace, sprint_trace);
  Result result;
  Run(*replay, olc_plus, result);
  delete replay;

  TextWriter writer("/dev/stdout", true);

  {
    JSON::ObjectWriter root(writer);

    WriteResult(root, result);
    root.WriteElement("contests", WriteContests, olc_plus.GetStats());
  }
}
