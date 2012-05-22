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

#include "Engine/Math/Earth.hpp"
#include "Engine/Trace/Trace.hpp"
#include "Contest/ContestManager.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "OS/Args.hpp"
#include "DebugReplay.hpp"
#include "NMEA/Aircraft.hpp"
#include "XML/Node.hpp"
#include "Util/Macros.hpp"
#include "IO/TextWriter.hpp"

static Trace full_trace(60, Trace::null_time, 256);
static Trace sprint_trace(0, 9000, 64);

static int
Run(DebugReplay &replay, ContestManager &contest)
{
  for (int i = 1; replay.Next(); i++) {
    const AircraftState state =
      ToAircraftState(replay.Basic(), replay.Calculated());
    full_trace.append(state);
    sprint_trace.append(state);
  }

  contest.SolveExhaustive();

  return 0;
}

static void
Add(XMLNode &parent, const TracePoint &point, const TracePoint *previous)
{
  XMLNode &node = parent.AddChild(_T("point"));

  StaticString<64> buffer;

  buffer.UnsafeFormat(_T("%u"), point.GetTime());
  node.AddAttribute(_T("time"), buffer.c_str());

  buffer.UnsafeFormat(_T("%f"),
                      (double)point.get_location().longitude.Degrees());
  node.AddAttribute(_T("longitude"), buffer.c_str());

  buffer.UnsafeFormat(_T("%f"),
                      (double)point.get_location().latitude.Degrees());
  node.AddAttribute(_T("latitude"), buffer.c_str());

  if (previous != NULL) {
    fixed distance = point.distance(previous->get_location());
    buffer.UnsafeFormat(_T("%u"), uround(distance));
    node.AddAttribute(_T("distance"), buffer.c_str());

    unsigned duration =
      std::max((int)point.GetTime() - (int)previous->GetTime(), 0);
    buffer.UnsafeFormat(_T("%u"), duration);
    node.AddAttribute(_T("duration"), buffer.c_str());

    if (duration > 0) {
      fixed speed = distance / duration;
      buffer.UnsafeFormat(_T("%1.2f"), (double)speed);
      node.AddAttribute(_T("speed"), buffer.c_str());
    }
  }
}

static void
Add(XMLNode &parent, const TCHAR *name,
    const ContestResult &result, const ContestTraceVector &trace)
{
  XMLNode &node = parent.AddChild(_T("trace"));
  node.AddAttribute(_T("name"), name);

  StaticString<64> buffer;

  buffer.UnsafeFormat(_T("%1.2f"), (double)result.score);
  node.AddAttribute(_T("score"), buffer.c_str());

  buffer.UnsafeFormat(_T("%1.2f"), (double)result.distance);
  node.AddAttribute(_T("distance"), buffer.c_str());

  buffer.UnsafeFormat(_T("%u"), uround(result.time));
  node.AddAttribute(_T("duration"), buffer.c_str());

  buffer.UnsafeFormat(_T("%1.2f"), (double)result.speed);
  node.AddAttribute(_T("speed"), buffer.c_str());

  const TracePoint *previous = NULL;
  for (auto i = trace.begin(), end = trace.end(); i != end; ++i) {
    Add(node, *i, previous);
    previous = &*i;
  }
}

static void
AddOLCPlus(XMLNode &parent, const ContestStatistics &stats)
{
  XMLNode &node = parent.AddChild(_T("contest"));
  node.AddAttribute(_T("name"), _T("olc_plus"));

  Add(node, _T("classic"), stats.result[0], stats.solution[0]);
  Add(node, _T("triangle"), stats.result[1], stats.solution[1]);
  Add(node, _T("plus"), stats.result[2], stats.solution[2]);
}

int main(int argc, char **argv)
{
  Args args(argc, argv, "DRIVER FILE");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  args.ExpectEnd();

  ContestManager olc_plus(OLC_Plus, full_trace, sprint_trace);
  Run(*replay, olc_plus);
  delete replay;

  XMLNode root = XMLNode::CreateRoot(_T("analysis"));
  AddOLCPlus(root, olc_plus.GetStats());

  TextWriter writer("/dev/stdout", true);
  root.Serialise(writer, true);
}
