// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TraceManager.hpp"
#include "Trace/Trace.hpp"

#include <cassert>

TraceManager::TraceManager(const Trace &_trace) noexcept
  :trace_master(_trace),
   predicted(TracePoint::Invalid())
{
}


bool
TraceManager::SetPredicted(const TracePoint &_predicted) noexcept
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
TraceManager::IsMasterUpdated(bool continuous) const noexcept
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
  const auto threshold_delta_t_trace = trace_master.GetAverageDeltaTime();
  const unsigned threshold_distance_trace = trace_master.GetAverageDeltaDistance();

  const TracePoint &last_master = trace_master.back();
  const TracePoint &last_point = *trace.back();

  // update trace if time and distance are greater than significance thresholds

  return last_master.GetTime() > last_point.GetTime() + threshold_delta_t_trace &&
    last_master.FlatDistanceTo(last_point) > threshold_distance_trace;
}

void
TraceManager::ClearTrace() noexcept
{
  append_serial = modify_serial = Serial();
  trace_dirty = true;
  trace.clear();
  n_points = 0;
  predicted = TracePoint::Invalid();
}

void
TraceManager::UpdateTraceFull() noexcept
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
TraceManager::UpdateTraceTail() noexcept
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
TraceManager::UpdateTrace([[maybe_unused]] bool force) noexcept
{
}


