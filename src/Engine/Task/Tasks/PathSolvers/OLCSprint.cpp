#include "OLCSprint.hpp"


OLCSprint::OLCSprint(OnlineContest& _olc):
  OLCDijkstra(_olc, 4, 0),
  m_start_limit(0),
  m_start_current(0)
{

}

bool 
OLCSprint::admit_candidate(const ScanTaskPoint &candidate) const
{
  if (m_reverse) {
    return (get_point(candidate).time+9000 >=
            solution[0].time) &&
      OLCDijkstra::admit_candidate(candidate);
  } else {
    return (get_point(candidate).time <=
            solution[0].time + 9000) &&
      OLCDijkstra::admit_candidate(candidate);
  }
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

/*
  @todo 
  - if solve completes, check against best solution
  - on completion, increment start edge counter
  - return true when start edge counter reaches limit
  - when to compute this limit?
 */
bool
OLCSprint::solve_inner()
{
  const bool solved = OLCDijkstra::solve_inner();
  const bool finished = (m_start_current >= m_start_limit);
  if (solved) {
    if (finished) {
      // find new start limit
      m_start_current = 0;
      m_start_limit = find_start_limit();
    }
    // @todo save best
  }
  return solved && finished;
}

/*
  @todo add only one start edge, each time, until get to 2.5 hours from end
 */
void
OLCSprint::add_start_edges()
{
  m_dijkstra.pop();

  ScanTaskPoint destination(0, m_start_current);
  m_dijkstra.link(destination, destination, 0);
  if (m_start_current<m_start_limit)
    m_start_current++;
}

unsigned
OLCSprint::find_start_limit() const
{
  ScanTaskPoint start(0,0);
  const ScanTaskPoint end(0, n_points-1);

  while (get_point(end).time-get_point(start).time > 9000) {
    start.second++;
  }
  return start.second;
}
