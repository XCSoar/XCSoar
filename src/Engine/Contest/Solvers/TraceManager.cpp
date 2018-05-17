/*
Copyright_License {

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

#include "TraceManager.hpp"
#include "Trace/Trace.hpp"

#include <assert.h>

TraceManager::TraceManager(const Trace &_trace)
  :trace_master(_trace),
   predicted(TracePoint::Invalid())
{
}


bool
TraceManager::SetPredicted(const TracePoint &_predicted)
{
  TracePoint n(_predicted);
  if (n.IsDefined() && !trace_master.empty())
    n.Project(trace_master.GetProjection());

  bool result = false;
  if (predicted.IsDefined() && n_points > 0) {
    if (n.IsDefined() && !trace_master.empty() &&
        n.GetFlatLocation() == predicted.GetFlatLocation()) {
      /* no change since last call, but update time stamp */
      predicted = n;
      return false;
    }

    /* the predicted location has changed, and the solver must be
       restarted */
    Reset();
    result = true;
  }

  predicted = n;
  return result;
}

bool
TraceManager::IsMasterUpdated(bool continuous) const
{
  /* disabled assertion, TraceManager doesn't know about stages */
  // assert(num_stages <= MAX_STAGES);

  if (CheckMasterSerial())
    return true;

  if (continuous)
    return false;

  if (trace.empty())
    return true;

  /* TODO: disabled check, move it to ContestDijkstra */
  //if (n_points < num_stages)
  //  return true;

  // find min distance and time step within this trace
  const unsigned threshold_delta_t_trace = trace_master.GetAverageDeltaTime();
  const unsigned threshold_distance_trace = trace_master.GetAverageDeltaDistance();

  const TracePoint &last_master = trace_master.back();
  const TracePoint &last_point = *trace.back();

  // update trace if time and distance are greater than significance thresholds

  return last_master.GetTime() > last_point.GetTime() + threshold_delta_t_trace &&
    last_master.FlatDistanceTo(last_point) > threshold_distance_trace;
}

void
TraceManager::ClearTrace()
{
  append_serial = modify_serial = Serial();
  trace_dirty = true;
  trace.clear();
  n_points = 0;
  predicted = TracePoint::Invalid();
}

void
TraceManager::UpdateTraceFull()
{
  trace.reserve(trace_master.GetMaxSize());
  trace_master.GetPoints(trace);
  n_points = trace.size();

  if (n_points > 0 && predicted.IsDefined())
    predicted.Project(trace_master.GetProjection());

  append_serial = trace_master.GetAppendSerial();
  modify_serial = trace_master.GetModifySerial();
}

bool
TraceManager::UpdateTraceTail()
{
  /* the following assertions were disabled because this method doesn't
     get the "force" and "continuous" parameter */
  //assert(continuous);
  //assert(incremental == finished || force);
  assert(modify_serial == trace_master.GetModifySerial());

  if (!trace_master.SyncPoints(trace))
    /* no new points */
    return false;

  n_points = trace.size();

  if (n_points > 0 && predicted.IsDefined())
    predicted.Project(trace_master.GetProjection());

  append_serial = trace_master.GetAppendSerial();
  return true;
}

void
TraceManager::UpdateTrace(bool force)
{
}


