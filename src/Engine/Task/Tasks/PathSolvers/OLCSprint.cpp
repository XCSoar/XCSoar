/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

OLCSprint::OLCSprint(const Trace &_trace,
                     const unsigned &_handicap):
  ContestDijkstra(_trace, _handicap, 4, 0) {}

void
OLCSprint::reset()
{
  ContestDijkstra::reset();
}

unsigned
OLCSprint::find_start() const
{
  assert(num_stages <= MAX_STAGES);
  assert(n_points > 0);
  ScanTaskPoint start(0, 1);
  const ScanTaskPoint end(0, n_points - 1);
  const unsigned end_time = get_point(end).time;
  if (end_time > 9000) {
    // fast forward to 2.5 hours before finish
    const unsigned start_time = end_time-9000;
    assert(start.point_index < n_points);
    while (get_point(start).time < start_time) {
      ++start.point_index;
      assert(start.point_index < n_points);
    }
  }

  return start.point_index;
}

void
OLCSprint::add_start_edges()
{
  assert(num_stages <= MAX_STAGES);
  assert(num_stages > 0);
  assert(n_points > 0);
  dijkstra.pop();

  const ScanTaskPoint start(0, find_start());
  const ScanTaskPoint finish(num_stages - 1, n_points - 1);

  solution[0] = get_point(start);

  if (admit_candidate(finish))
    dijkstra.link(start, start, 0);
}

void 
OLCSprint::add_edges(const ScanTaskPoint &origin)
{
  const ScanTaskPoint destination(origin.stage_number + 1, n_points - 1);
  if (!is_final(destination)) {
    ContestDijkstra::add_edges(origin);
    return;
  }
  /*
    For final, only add last valid point
   */
  const unsigned d = get_weighting(origin.stage_number) *
    distance(origin, destination);
  dijkstra.link(destination, origin, d);
}

fixed
OLCSprint::calc_score() const
{
  return apply_handicap(calc_distance()/fixed(2500), true);
}

void
OLCSprint::update_trace() {

  // since this is online, all solutions must have start to end of trace
  // satisfy the finish altitude requirements.  otherwise there is no point
  // even retrieving the full trace or starting a search.

  // assuming a bounded ceiling and very long flight, this would be expected to reduce
  // the number of trace acquisitions and solution starts by 50%.  In practice the number
  // will be lower than this but the fewer wasted cpu cycles the better.

  TracePointVector e;
  trace_master.get_trace_edges(e);

  if ((e.size()<2) || !finish_altitude_valid(e[0], e[1])) {
    clear_trace();
    return;
  }

  ContestDijkstra::update_trace();
}
