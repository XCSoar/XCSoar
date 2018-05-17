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

#include "ContestDijkstra.hpp"
#include "../ContestResult.hpp"
#include "Trace/Trace.hpp"
#include "Cast.hpp"

#include <algorithm>
#include <assert.h>

// set size of reserved queue elements (may differ from Dijkstra default)
static constexpr unsigned CONTEST_QUEUE_SIZE = 5000;

ContestDijkstra::ContestDijkstra(const Trace &_trace,
                                 bool _continuous,
                                 const unsigned n_legs,
                                 const unsigned finish_alt_diff)
  :AbstractContest(finish_alt_diff),
   NavDijkstra(n_legs + 1),
   TraceManager(_trace),
   continuous(_continuous),
   incremental(false)
{
  assert(num_stages <= MAX_STAGES);

  std::fill_n(stage_weights, num_stages - 1, 5);
}

void
ContestDijkstra::UpdateTrace(bool force)
{
  if (IsMasterAppended()) return; /* unmodified */

  if (IsMasterUpdated(continuous)) {
    UpdateTraceFull();

    trace_dirty = true;
    finished = false;

    first_finish_candidate = incremental ? n_points - 1 : 0;
  } else if (finished) {
    const unsigned old_size = n_points;
    if (UpdateTraceTail())
      /* new data from the master trace, start incremental solver */
      AddIncrementalEdges(old_size);
  } else if (force) {
    if (incremental && continuous) {
      if (UpdateTraceTail()) {
        /* new data from the master trace, restart the non-incremental
           solver */
        trace_dirty = true;
        first_finish_candidate = incremental ? n_points - 1 : 0;
      }
    } else {
      UpdateTraceFull();

      trace_dirty = true;
      finished = false;

      first_finish_candidate = incremental ? n_points - 1 : 0;
    }
  }
}

SolverResult
ContestDijkstra::Solve(bool exhaustive)
{
  assert(num_stages <= MAX_STAGES);

  if (trace_master.size() < num_stages) {
    /* not enough data in master trace */
    ClearTrace();
    finished = false;
    return SolverResult::FAILED;
  }

  if (finished || dijkstra.IsEmpty()) {
    UpdateTrace(exhaustive);

    if (n_points < num_stages)
      return SolverResult::FAILED;

    // don't re-start search unless we have had new data appear
    if (!trace_dirty && !finished)
      return SolverResult::FAILED;
  } else if (exhaustive || n_points < num_stages ||
             CheckMasterSerial()) {
    UpdateTrace(exhaustive);
    if (n_points < num_stages)
      return SolverResult::FAILED;
  }

  assert(n_points >= num_stages);

  if (trace_dirty) {
    trace_dirty = false;
    finished = false;

    dijkstra.Clear();
    dijkstra.Reserve(CONTEST_QUEUE_SIZE);

    StartSearch();
    AddStartEdges();
    if (dijkstra.IsEmpty())
      return SolverResult::FAILED;
  }

  SolverResult result = DistanceGeneral(exhaustive ? 0 - 1 : 25);
  if (result != SolverResult::INCOMPLETE) {
    if (incremental && continuous)
      /* enable the incremental solver, which considers the existing
         Dijkstra edge map */
      finished = true;
    else
      /* start the next iteration from scratch */
      dijkstra.Clear();

    if (result == SolverResult::VALID && !SaveSolution())
      result = SolverResult::FAILED;
  }

  return result;
}

void
ContestDijkstra::Reset()
{
  dijkstra.Clear();
  ClearTrace();
  finished = false;

  AbstractContest::Reset();
}

bool
ContestDijkstra::SaveSolution()
{
  solution.resize(num_stages);

  for (unsigned i = 0; i < num_stages; ++i)
    solution[i] =
      NavDijkstra::solution[i] == predicted_index
      ? (predicted.IsDefined()
         ? predicted
         /* fallback, just in case somebody has deleted the prediction
            meanwhile */
         : TraceManager::GetPoint(n_points - 1))
      : TraceManager::GetPoint(NavDijkstra::solution[i]);

  return AbstractContest::SaveSolution();
}

ContestResult
ContestDijkstra::CalculateResult(const ContestTraceVector &solution) const
{
  assert(num_stages <= MAX_STAGES);

  ContestResult result;
  result.time = solution[num_stages - 1].DeltaTime(solution[0]);
  result.distance = result.score = 0;

  GeoPoint previous = solution[0].GetLocation();
  for (unsigned i = 1; i < num_stages; ++i) {
    const GeoPoint &current = solution[i].GetLocation();
    result.distance += current.Distance(previous);
    result.score += GetStageWeight(i - 1) * current.Distance(previous);
    previous = current;
  }

  #define FIFTH 0.0002
  result.score *= FIFTH;
  result.score = ApplyHandicap(result.score);

  return result;
}

ContestResult
ContestDijkstra::CalculateResult() const
{
  return CalculateResult(solution);
}

void
ContestDijkstra::AddStartEdges()
{
  assert(num_stages <= MAX_STAGES);
  assert(n_points > 0);

  const int max_altitude = incremental
    ? GetMaximumStartAltitude(TraceManager::GetPoint(n_points - 1))
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

  bool previous_above = false;
  for (const ScanTaskPoint end(destination.GetStageNumber(), n_points);
       destination != end; destination.IncrementPointIndex()) {
    bool above = GetPoint(destination).GetIntegerAltitude() >= min_altitude;

    if (above) {
      const unsigned d = weight * CalcEdgeDistance(origin, destination);
      Link(destination, origin, d);
    } else if (previous_above) {
      /* After excessive thinning, the exact TracePoint that matches
         the required altitude difference may be gone, and the
         calculated result becomes overly pessimistic.  This code path
         makes it optimistic, by checking if the previous point
         matches. */

      /* TODO: interpolate the distance */
      const unsigned d = weight * CalcEdgeDistance(origin, destination);
      Link(destination, origin, d);
    }

    previous_above = above;
  }

  if (IsFinal(destination) && predicted.IsDefined()) {
    const unsigned d = weight * GetPoint(origin).FlatDistanceTo(predicted);
    destination.SetPointIndex(predicted_index);
    Link(destination, origin, d);
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
#if GCC_VERSION < 40800 || GCC_VERSION > 40802
  const Dijkstra::EdgeMap edges = dijkstra.GetEdgeMap();
#else
  // workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=59548
  Dijkstra::EdgeMap edges;
  edges = dijkstra.GetEdgeMap();
#endif

  /* establish links between each old node and each new node, to
     initiate the follow-up search, hoping a better solution will be
     found here */
  for (const auto &i : edges) {
    if (IsFinal(i.first))
      /* ignore final nodes */
      continue;

    /* "seek" the Dijkstra object to the current "old" node */
    dijkstra.SetCurrentValue(i.second.value);

    /* add edges from the current "old" node to all "new" nodes
       (first_point .. n_points-1) */
    AddEdges(i.first, first_point);
  }

  /* see if new start points are possible now (due to relaxed start
     height constraints); duplicates will be ignored by the Dijkstra
     class */
  AddStartEdges();
}

void
ContestDijkstra::CopySolution(ContestTraceVector &result) const
{
  assert(num_stages <= MAX_STAGES);

  result = solution;
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
