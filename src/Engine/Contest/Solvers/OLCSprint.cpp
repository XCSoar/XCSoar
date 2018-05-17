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

#include "OLCSprint.hpp"
#include "Trace/Trace.hpp"

/*
  - note, this only searches 2.5 hour blocks, so should be able
    to handle larger number of points than other ContestDijkstra's
  - Therefore, consider having the trace points stored within
    this instance instead of in ContestManager.
  - Solutions must be improving as time goes on, so no need to search
    back from current best
  - with sampling at approx 300 points, 2.5 hours = 1pt/30s
    .: to keep ahead, will need to be able to find a solution within
     30s at 300pt resolution, otherwise solver will lag behind new data.
  - is there an implicit assumption that start is lowest point?

  - if this is up to date, no need to process anything earlier than
    last 2.5 hours, since save_solution will catch the very best

  - only need to pass in last 2.5 hours worth of data, therefore
    use min_time and have this class request data directly from Trace

  - so, calculate acceptable n_points size so we get a solution
    on slow platforms within a close-to-one fraction of the
    resulting time step for new points
    e.g. if n_points is 150 (one per minute), we expect a solution
    in approx worst case within 60 cycles.

    we want n_points as large as possible to reduce error.

    potentially implement as circular buffer (emulate as dequeue)
*/

OLCSprint::OLCSprint(const Trace &_trace)
  :ContestDijkstra(_trace, false, 4, 0) {}

unsigned
OLCSprint::FindStart() const
{
  assert(num_stages <= MAX_STAGES);
  assert(n_points >= 2);

  unsigned start_index = 0;
  const auto end_time = TraceManager::GetPoint(n_points - 1).GetTime();
  if (end_time > 9000) {
    // fast forward to 2.5 hours before finish
    const unsigned start_time = end_time-9000;
    assert(start_index < n_points);
    while (TraceManager::GetPoint(start_index).GetTime() < start_time) {
      ++start_index;
      assert(start_index < n_points);
    }
  }

  return start_index;
}

void
OLCSprint::AddStartEdges()
{
  assert(num_stages <= MAX_STAGES);
  assert(num_stages > 0);
  assert(n_points > 0);

  const int max_altitude = GetMaximumStartAltitude(TraceManager::GetPoint(n_points - 1));

  const ScanTaskPoint start(0, FindStart());

  if (GetPoint(start).GetIntegerAltitude() <= max_altitude)
    LinkStart(start);
}

void
OLCSprint::AddEdges(const ScanTaskPoint origin)
{
  const ScanTaskPoint destination(origin.GetStageNumber() + 1, n_points - 1);
  if (IsFinal(destination)) {
    /* For final, only add last valid point */
    const unsigned d = GetStageWeight(origin.GetStageNumber()) *
      CalcEdgeDistance(origin, destination);
    Link(destination, origin, d);
  } else
    ContestDijkstra::AddEdges(origin);
}

ContestResult
OLCSprint::CalculateResult() const
{
  ContestResult result = ContestDijkstra::CalculateResult();
  result.score = ApplyShiftedHandicap(result.distance / 2500.);
  return result;
}

void
OLCSprint::UpdateTrace(bool force)
{
  /* since this is online, all solutions must have start to end of
     trace satisfy the finish altitude requirements.  otherwise there
     is no point even retrieving the full trace or starting a
     search. */

  /* assuming a bounded ceiling and very long flight, this would be
     expected to reduce the number of trace acquisitions and solution
     starts by 50%.  In practice the number will be lower than this
     but the fewer wasted cpu cycles the better. */

  if (trace_master.size() < 2) {
    ClearTrace();
    return;
  }

  const TracePoint &first = trace_master.front();
  const TracePoint &last = trace_master.back();

  if (!IsFinishAltitudeValid(first, last)) {
    ClearTrace();
    return;
  }

  ContestDijkstra::UpdateTrace(force);
}
