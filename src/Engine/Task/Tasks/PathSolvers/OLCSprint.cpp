#include "OLCSprint.hpp"

#ifdef DO_PRINT
#include <stdio.h>
#endif

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

OLCSprint::OLCSprint(const TracePointVector &_trace):
  ContestDijkstra(_trace, 4, 0) {}

void
OLCSprint::reset()
{
  ContestDijkstra::reset();
}

unsigned
OLCSprint::find_start() const
{
  ScanTaskPoint start(0, 1);
  const ScanTaskPoint end(0, n_points - 1);
  const unsigned end_time = get_point(end).time;
  if (end_time > 9000) {
    // fast forward to 2.5 hours before finish
    const unsigned start_time = end_time-9000;
    while (get_point(start).time < start_time)
      ++start.second;
  }

  return start.second;
}

void
OLCSprint::add_start_edges()
{
  m_dijkstra.pop();

  const ScanTaskPoint start(0, find_start());
  const ScanTaskPoint finish(num_stages - 1, n_points - 1);

  solution[0] = get_point(start);

  if (admit_candidate(finish))
    m_dijkstra.link(start, start, 0);
}

void 
OLCSprint::add_edges(DijkstraTaskPoint &dijkstra,
                     const ScanTaskPoint &origin)
{
  const ScanTaskPoint destination(origin.first + 1, n_points - 1);
  if (!is_final(destination)) {
    ContestDijkstra::add_edges(dijkstra, origin);
    return;
  }
  /*
    For final, only add last valid point
   */
  const unsigned d = get_weighting(origin.first) *
    distance(origin, destination);
  dijkstra.link(destination, origin, d);
}

fixed
OLCSprint::calc_score() const
{
  // @todo: apply handicap *(200/(100+handicap)
  return calc_distance()/fixed(2500);
}
