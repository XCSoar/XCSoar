#include "OLCSprint.hpp"

#ifdef DO_PRINT
#include <stdio.h>
#endif

/*
 @todo
  - note, this only searches 2.5 hour blocks, so should be able
    to handle larger number of points than other OLCDijkstra's
  - Therefore, consider having the trace points stored within
    this instance instead of in OnlineContest.
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
*/

OLCSprint::OLCSprint(OnlineContest& _olc):
  OLCDijkstra(_olc, 4, 0)
{

}

void
OLCSprint::reset()
{
  OLCDijkstra::reset();
}

bool 
OLCSprint::admit_candidate(const ScanTaskPoint &candidate) const
{
  return (get_point(candidate).time <=
          solution[0].time + 9000) &&
    OLCDijkstra::admit_candidate(candidate);
}

fixed
OLCSprint::score(fixed& the_distance)  
{
  static const fixed fixed_9000(9000);

  const fixed dist = OLCDijkstra::score(the_distance);

  if (positive(dist)) {
    fixed time = calc_time();
    if (positive(time)) {
      return dist/time;
    }
  }
  return fixed_zero;

//  return dist/max(fixed_9000, time);
}


unsigned
OLCSprint::find_start() const
{
  ScanTaskPoint start(0,1);
  const ScanTaskPoint end(0, n_points-1);

  while (get_point(start).time + 9000 < get_point(end).time) {
    start.second++;
  }
  return start.second;
}

void
OLCSprint::add_start_edges()
{
  m_dijkstra.pop();

  ScanTaskPoint start(0, find_start());
  ScanTaskPoint finish(num_stages-1, n_points-1);

  if (admit_candidate(finish)) {
    m_dijkstra.link(start, start, 0);
  }
}

void 
OLCSprint::add_edges(DijkstraTaskPoint &dijkstra,
                     const ScanTaskPoint &origin)
{
  ScanTaskPoint destination(origin.first+1, origin.second);
  if (!is_final(destination)) {
    OLCDijkstra::add_edges(dijkstra, origin);
    return;
  }

/*
  For final, only add last valid point
 */

  find_solution(dijkstra, origin);
  
  for (int p=destination.second; 
       p>= (int)origin.second; --p) {
    destination.second = p;
    if (admit_candidate(destination)) {
      const unsigned d = get_weighting(origin.first)
        *distance(origin, destination);
      dijkstra.link(destination, origin, d);
      return;
    }
  }
}
