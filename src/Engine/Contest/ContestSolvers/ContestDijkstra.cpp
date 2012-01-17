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

#ifdef INSTRUMENT_TASK
unsigned long ContestDijkstra::count_olc_solve = 0;
unsigned long ContestDijkstra::count_olc_trace = 0;
unsigned ContestDijkstra::count_olc_size = 0;
#endif

// set size of reserved queue elements (may differ from Dijkstra default)
#define CONTEST_QUEUE_SIZE DIJKSTRA_QUEUE_SIZE

ContestDijkstra::ContestDijkstra(const Trace &_trace,
                                 const unsigned n_legs,
                                 const unsigned finish_alt_diff):
  AbstractContest(_trace, finish_alt_diff),
  NavDijkstra(false, n_legs + 1)
{
  assert(num_stages <= MAX_STAGES);

  std::fill(m_weightings, m_weightings + num_stages - 1, 5);

  Reset();
}

bool
ContestDijkstra::Score(ContestResult &result)
{
  assert(num_stages <= MAX_STAGES);

  return n_points >= num_stages && AbstractContest::Score(result);
}

bool
ContestDijkstra::master_is_updated() const
{
  assert(num_stages <= MAX_STAGES);

  if (modify_serial != trace_master.GetModifySerial())
    return true;

  if (n_points < num_stages)
    return true;

  // find min distance and time step within this trace
  const unsigned threshold_delta_t_trace = trace_master.average_delta_time();
  const unsigned threshold_distance_trace = trace_master.average_delta_distance();

  const TracePoint &last_master = trace_master.back();
  const TracePoint &last_point = trace.back();

  // update trace if time and distance are greater than significance thresholds

  return last_master.GetTime() > last_point.GetTime() + threshold_delta_t_trace &&
    last_master.flat_distance(last_point) > threshold_distance_trace;
}


void
ContestDijkstra::clear_trace()
{
  trace_dirty = true;
  trace.clear();
  n_points = 0;
}


void
ContestDijkstra::update_trace()
{
  if (!master_is_updated())
    return;

  trace.reserve(trace_master.GetMaxSize());
  trace_master.get_trace_points(trace);
  modify_serial = trace_master.GetModifySerial();
  n_points = trace.size();
  trace_dirty = true;

#ifdef INSTRUMENT_TASK
  count_olc_trace++;
#endif

  if (n_points<2) return;
}


bool
ContestDijkstra::Solve(bool exhaustive)
{
  assert(num_stages <= MAX_STAGES);

  if (trace_master.size() < num_stages) {
    /* not enough data in master trace */
    clear_trace();
    return true;
  }

  if (dijkstra.empty()) {

    update_trace();
    if (n_points < num_stages)
      return true;

    // don't re-start search unless we have had new data appear
    if (!trace_dirty) {
      return true;
    }
  } else if (exhaustive || n_points < num_stages ||
             modify_serial != trace_master.GetModifySerial()) {
    update_trace();
    if (n_points < num_stages)
      return true;
  }

  assert(n_points >= num_stages);

  if (trace_dirty) {
    trace_dirty = false;

    dijkstra.clear();
    dijkstra.reserve(CONTEST_QUEUE_SIZE);

    start_search();
    add_start_edges();
    if (dijkstra.empty()) {
      return true;
    }
  }

#ifdef INSTRUMENT_TASK
  count_olc_solve++;
  count_olc_size = max(count_olc_size, dijkstra.queue_size());
#endif

  if (distance_general(exhaustive ? 0 - 1 : 25)) {
    SaveSolution();
    update_trace();
    return true;
  }

  return !dijkstra.empty();
}

void
ContestDijkstra::Reset()
{
  best_solution.clear();
  dijkstra.clear();
  solution_valid = false;
  clear_trace();

  AbstractContest::Reset();

#ifdef INSTRUMENT_TASK
  count_olc_solve = 0;
  count_olc_trace = 0;
  count_olc_size = 0;
#endif
}


fixed
ContestDijkstra::CalcTime() const
{
  assert(num_stages <= MAX_STAGES);

  if (!solution_valid)
    return fixed_zero;

  return fixed(GetPoint(solution[num_stages - 1])
               .DeltaTime(GetPoint(solution[0])));
}

fixed
ContestDijkstra::CalcDistance() const
{
  assert(num_stages <= MAX_STAGES);

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

  fixed score = fixed_zero;
  GeoPoint previous = GetPoint(solution[0]).get_location();
  for (unsigned i = 1; i < num_stages; ++i) {
    const GeoPoint &current = GetPoint(solution[i]).get_location();
    score += get_weighting(i - 1) * current.Distance(previous);
    previous = current;
  }

  #define fixed_fifth fixed(0.0002)
  score *= fixed_fifth;

  return ApplyHandicap(score);
}

void
ContestDijkstra::add_start_edges()
{
  assert(num_stages <= MAX_STAGES);
  assert(n_points > 0);

  ScanTaskPoint destination(0, 0);
  const ScanTaskPoint finish(num_stages-1, n_points-1);

  for (ScanTaskPoint destination(0, 0), end(0, n_points);
       destination != end; destination.IncrementPointIndex()) {
    // only add points that are valid for the finish
    if (admit_candidate(GetPointFast(destination), finish)) {
      dijkstra.link(destination, destination, 0);
    }
  }
}

void
ContestDijkstra::add_edges(const ScanTaskPoint origin)
{
  ScanTaskPoint destination(origin.GetStageNumber() + 1,
                            origin.GetPointIndex());

  const TracePoint start = GetPointFast(FindStart(origin));

  // only add last point!
  if (is_final(destination)) {
    assert(n_points > 0);
    destination.SetPointIndex(n_points - 1);
  }

  const unsigned weight = get_weighting(origin.GetStageNumber());

  for (const ScanTaskPoint end(destination.GetStageNumber(), n_points);
       destination != end; destination.IncrementPointIndex()) {
    if (admit_candidate(start, destination)) {
      const unsigned d = weight * distance(origin, destination);
      dijkstra.link(destination, origin, d);
    }
  }
}

bool
ContestDijkstra::admit_candidate(const TracePoint &start,
                                 const ScanTaskPoint candidate) const
{
  if (!is_final(candidate))
    return true;
  else
    return IsFinishAltitudeValid(start, GetPointFast(candidate));
}

bool
ContestDijkstra::SaveSolution()
{
  assert(num_stages <= MAX_STAGES);

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
ContestDijkstra::start_search()
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
