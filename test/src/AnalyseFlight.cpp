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
#include "JSON/json.hpp"

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
Add(json::Object &object, const GeoPoint &gp)
{
  object.Add(_T("longitude"), (double)gp.longitude.Degrees());
  object.Add(_T("latitude"), (double)gp.latitude.Degrees());
}

static void
Add(json::Object &root, const Result &result,
    const MoreData &basic, const DerivedInfo &calculated)
{
  StaticString<64> buffer;

  auto times = new json::Object();
  root.Add(_T("times"), times);

  if (result.takeoff_time.Plausible()) {
    FormatISO8601(buffer.buffer(), result.takeoff_time);
    times->Add(_T("takeoff"), buffer.c_str());
  }

  if (result.landing_time.Plausible()) {
    FormatISO8601(buffer.buffer(), result.landing_time);
    times->Add(_T("landing"), buffer.c_str());
  }

  auto locations = new json::Object();
  root.Add(_T("locations"), locations);

  if (result.takeoff_location.IsValid()) {
    auto node = new json::Object();
    locations->Add(_T("takeoff"), node);

    Add(*node, result.takeoff_location);
  }

  if (result.landing_location.IsValid()) {
    auto node = new json::Object();
    locations->Add(_T("landing"), node);

    Add(*node, result.landing_location);
  }
}

static void
Add(json::Array &parent, const TracePoint &point, const TracePoint *previous)
{
  auto node = new json::Object();
  parent.Add(node);

  node->Add(_T("time"), (long)point.GetTime());
  node->Add(_T("longitude"), (double)point.get_location().longitude.Degrees());
  node->Add(_T("latitude"), (double)point.get_location().latitude.Degrees());

  if (previous != NULL) {
    fixed distance = point.distance(previous->get_location());
    node->Add(_T("distance"), (long)uround(distance));

    unsigned duration =
      std::max((int)point.GetTime() - (int)previous->GetTime(), 0);
    node->Add(_T("duration"), (long)duration);

    if (duration > 0) {
      fixed speed = distance / duration;
      node->Add(_T("speed"), (double)speed);
    }
  }
}

static void
Add(json::Array &parent, const ContestTraceVector &trace)
{
  const TracePoint *previous = NULL;
  for (auto i = trace.begin(), end = trace.end(); i != end; ++i) {
    Add(parent, *i, previous);
    previous = &*i;
  }
}

static void
Add(json::Object &parent, const TCHAR *name,
    const ContestResult &result, const ContestTraceVector &trace)
{
  auto node = new json::Object();
  parent.Add(name, node);

  node->Add(_T("score"), (double)result.score);
  node->Add(_T("distance"), (double)result.distance);
  node->Add(_T("duration"), (long)uround(result.time));
  node->Add(_T("speed"), (double)result.GetSpeed());

  auto turnpoints = new json::Array();
  node->Add(_T("turnpoints"), turnpoints);
  Add(*turnpoints, trace);
}

static void
AddOLCPlus(json::Object &parent, const ContestStatistics &stats)
{
  auto node = new json::Object();
  parent.Add(_T("olc_plus"), node);

  Add(*node, _T("classic"), stats.result[0], stats.solution[0]);
  Add(*node, _T("triangle"), stats.result[1], stats.solution[1]);
  Add(*node, _T("plus"), stats.result[2], stats.solution[2]);
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

  json::Object root;
  Add(root, result, replay->Basic(), replay->Calculated());
  delete replay;

  auto contests = new json::Object();
  root.Add(_T("contests"), contests);

  AddOLCPlus(*contests, olc_plus.GetStats());

  TextWriter writer("/dev/stdout", true);
  root.Serialise(writer, -1);
}
