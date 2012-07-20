/*
Copyright_License {

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

#include "ContestDijkstra.hpp"
#include "../ContestResult.hpp"
#include "Trace/Trace.hpp"

#include <algorithm>
#include <assert.h>
#include <limits.h>

// set size of reserved queue elements (may differ from Dijkstra default)
static gcc_constexpr_data unsigned CONTEST_QUEUE_SIZE = 5000;

ContestDijkstra::ContestDijkstra(const Trace &_trace,
                                 bool _continuous,
                                 const unsigned n_legs,
                                 const unsigned finish_alt_diff):
  AbstractContest(_trace, finish_alt_diff),
  NavDijkstra(n_legs + 1),
  continuous(_continuous),
  incremental(false)
{
  assert(num_stages <= MAX_STAGES);

  std::fill(m_weightings, m_weightings + num_stages - 1, 5);

  Reset();
}

bool
ContestDijkstra::Score(ContestResult &result)
{
  assert(num_stages <= MAX_STAGES);

  return n_points >= num_stages && solution_valid &&
    AbstractContest::Score(result);
}

bool
ContestDijkstra::IsMasterUpdated() const
{
  assert(num_stages <= MAX_STAGES);

  if (modify_serial != trace_master.GetModifySerial())
    return true;

  if (continuous)
    return false;

  if (n_points < num_stages)
    return true;

  // find min distance and time step within this trace
  const unsigned threshold_delta_t_trace = trace_master.average_delta_time();
  const unsigned threshold_distance_trace = trace_master.average_delta_distance();

  const TracePoint &last_master = trace_master.back();
  const TracePoint &last_point = *trace.back();

  // update trace if time and distance are greater than significance thresholds

  return last_master.GetTime() > last_point.GetTime() + threshold_delta_t_trace &&
    last_master.flat_distance(last_point) > threshold_distance_trace;
}


void
ContestDijkstra::ClearTrace()
{
  append_serial = modify_serial = Serial();
  trace_dirty = true;
  finished = false;
  trace.clear();
  n_points = 0;
}

bool
ContestDijkstra::UpdateTraceTail()
{
  assert(continuous);
  assert(incremental);
  assert(finished);
  assert(modify_serial == trace_master.GetModifySerial());

  if (!trace_master.SyncTracePoints(trace))
    /* no new points */
    return false;

  n_points = trace.size();
  return true;
}

void
ContestDijkstra::UpdateTrace()
{
  if (!IsMasterUpdated()) {
    if (finished && append_serial != trace_master.GetAppendSerial()) {
      const unsigned old_size = n_points;
      if (UpdateTraceTail())
        /* new data from the master trace, start incremental solver */
        AddIncrementalEdges(old_size);
    }

    return;
  }

  trace.reserve(trace_master.GetMaxSize());
  trace_master.GetTracePoints(trace);
  append_serial = trace_master.GetAppendSerial();
  modify_serial = trace_master.GetModifySerial();
  n_points = trace.size();
  trace_dirty = true;
  finished = false;

  first_finish_candidate = incremental ? n_points - 1 : 0;

  if (n_points<2) return;
}


bool
ContestDijkstra::Solve(bool exhaustive)
{
  assert(num_stages <= MAX_STAGES);

  solution_valid = false;

  if (trace_master.size() < num_stages) {
    /* not enough data in master trace */
    ClearTrace();
    return true;
  }

  if (finished || dijkstra.IsEmpty()) {
    UpdateTrace();

    if (n_points < num_stages)
      return true;

    // don't re-start search unless we have had new data appear
    if (!trace_dirty && !finished)
      return true;
  } else if (exhaustive || n_points < num_stages ||
             modify_serial != trace_master.GetModifySerial()) {
    UpdateTrace();
    if (n_points < num_stages)
      return true;
  }

  assert(n_points >= num_stages);

  if (trace_dirty) {
    trace_dirty = false;
    finished = false;

    dijkstra.Clear();
    dijkstra.Reserve(CONTEST_QUEUE_SIZE);

    StartSearch();
    AddStartEdges();
    if (dijkstra.IsEmpty()) {
      return true;
    }
  }

  if (DistanceGeneral(exhaustive ? 0 - 1 : 25)) {
    if (incremental && continuous)
      /* enable the incremental solver, which considers the existing
         Dijkstra edge map */
      finished = true;
    else
      /* start the next iteration from scratch */
      dijkstra.Clear();

    if (solution_valid)
      SaveSolution();

    UpdateTrace();
    return true;
  }

  return !dijkstra.IsEmpty();
}

void
ContestDijkstra::Reset()
{
  best_solution.clear();
  dijkstra.Clear();
  solution_valid = false;
  ClearTrace();

  AbstractContest::Reset();
}


fixed
ContestDijkstra::CalcTime() const
{
  assert(num_stages <= MAX_STAGES);
  assert(solution_valid);

  return fixed(GetPoint(solution[num_stages - 1])
               .DeltaTime(GetPoint(solution[0])));
}

fixed
ContestDijkstra::CalcDistance() const
{
  assert(num_stages <= MAX_STAGES);
  assert(solution_valid);

  fixed dist = fixed_zero;
  GeoPoint previous = GetPoint(solution[0]).get_location();
  for (unsigned i = 1; i < num_stages; ++i) {
    const GeoPoint &current = GetPoint(solution[i]).get_location();
    dist += current.Distance(previous);
    previous = current;
  }

  return dist;
}

fixed
ContestDijkstra::CalcScore() const
{
  assert(num_stages <= MAX_STAGES);
  assert(solution_valid);

  fixed score = fixed_zero;
  GeoPoint previous = GetPoint(solution[0]).get_location();
  for (unsigned i = 1; i < num_stages; ++i) {
    const GeoPoint &current = GetPoint(solution[i]).get_location();
    score += GetStageWeight(i - 1) * current.Distance(previous);
    previous = current;
  }

  #define fixed_fifth fixed(0.0002)
  score *= fixed_fifth;

  return ApplyHandicap(score);
}

void
ContestDijkstra::AddStartEdges()
{
  assert(num_stages <= MAX_STAGES);
  assert(n_points > 0);

  const int max_altitude = incremental
    ? GetMaximumStartAltitude(GetPoint(n_points - 1))
    : 0;

  for (ScanTaskPoint destination(0, 0), end(0, n_points);
       destination != end; destination.IncrementPointIndex()) {
    // only add points that are valid for the finish
    if (!incremental ||
        GetPoint(destination).GetIntegerAltitude() <= max_altitude)
      LinkStart(destination);
  }
}

void
ContestDijkstra::AddEdges(const ScanTaskPoint origin,
                          const unsigned first_point)
{
  ScanTaskPoint destination(origin.GetStageNumber() + 1,
                            std::max(origin.GetPointIndex(), first_point));

  const int min_altitude = IsFinal(destination)
    ? GetMinimumFinishAltitude(GetPoint(FindStart(origin)))
    : 0;

  if (IsFinal(destination) &&
      first_finish_candidate > destination.GetPointIndex())
    /* limit the number of finish candidates during incremental
       search */
    destination.SetPointIndex(first_finish_candidate);

  const unsigned weight = GetStageWeight(origin.GetStageNumber());

  for (const ScanTaskPoint end(destination.GetStageNumber(), n_points);
       destination != end; destination.IncrementPointIndex()) {
    if (GetPoint(destination).GetIntegerAltitude() >= min_altitude) {
      const unsigned d = weight * CalcEdgeDistance(origin, destination);
      Link(destination, origin, d);
    }
  }

}

void
ContestDijkstra::AddEdges(const ScanTaskPoint origin)
{
  AddEdges(origin, 0);
}

void
ContestDijkstra::AddIncrementalEdges(unsigned first_point)
{
  assert(first_point < n_points);
  assert(continuous);
  assert(incremental);
  assert(finished);

  finished = false;
  first_finish_candidate = first_point;

  /* we need a copy of the current edge map, because the following
     loop will modify it, invalidating the iterator */
  const Dijkstra::EdgeMap edges = dijkstra.GetEdgeMap();

  /* establish links between each old node and each new node, to
     initiate the follow-up search, hoping a better solution will be
     found here */
  for (auto i = edges.begin(), end = edges.end(); i != end; ++i) {
    if (IsFinal(i->first))
      /* ignore final nodes */
      continue;

    /* "seek" the Dijkstra object to the current "old" node */
    dijkstra.SetCurrentValue(i->second.value);

    /* add edges from the current "old" node to all "new" nodes
       (first_point .. n_points-1) */
    AddEdges(i->first, first_point);
  }

  /* see if new start points are possible now (due to relaxed start
     height constraints); duplicates will be ignored by the Dijkstra
     class */
  AddStartEdges();
}

bool
ContestDijkstra::SaveSolution()
{
  assert(num_stages <= MAX_STAGES);
  assert(solution_valid);

  if (solution_valid && AbstractContest::SaveSolution()) {
    best_solution.clear();
    for (unsigned i=0; i<num_stages; ++i) {
      best_solution.append(GetPoint(solution[i]));
    }
    return true;
  }
  return false;
}


void
ContestDijkstra::CopySolution(ContestTraceVector &vec) const
{
  assert(num_stages <= MAX_STAGES);

  vec = best_solution;
}

void 
ContestDijkstra::StartSearch()
{
  // nothing required by default
}




/*

OLC classic:
- start, 5 turnpoints, finish
- weightings: 1,1,1,1,0.8,0.6

FAI OLC:
- start, 2 turnpoints, finish
- if d<500km, shortest leg >= 28% d; else shortest leg >= 25%d
- considered closed if a fix is within 1km of starting point
- weightings: 1 all

OLC classic + FAI-OLC
- min finish alt is 1000m below start altitude 
- start altitude is lowest altitude before reaching start
- start time is time at which start altitude is reached
- The finish altitude is the highest altitude after reaching the finish point and before end of free flight.
- The finish time is the time at which the finish altitude is reached after the finish point is reached.

OLC league:
- start, 3 turnpoints, finish
- weightings: 1 all
- The sprint start point can not be higher than the sprint end point.
- The sprint start altitude is the altitude at the sprint start point.
- Sprint arrival height is the altitude at the sprint end point.
- The average speed (points) of each individual flight is the sum of
  the distances from sprint start, around up to three turnpoints, to the
  sprint end divided DAeC index increased by 100, multiplied by 200 and
  divided by 2.5h: [formula: Points = km / 2,5 * 200 / (Index+100)

*/
