#include "OLCSprint.hpp"


OLCSprint::OLCSprint(OnlineContest& _olc):
  OLCDijkstra(_olc, 4, 0),
  m_start_limit(0),
  m_start_current(0)
{

}

void
OLCSprint::reset()
{
  OLCDijkstra::reset();
  m_start_limit = 0;
  m_start_current = 0;
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

bool
OLCSprint::solve_inner()
{
  if (OLCDijkstra::solve_inner()) {
    // @todo save best    
    return (m_start_current==0);
  } else {
    return false;
  }
}

/*
  add only one start edge, each time
 */
void
OLCSprint::add_start_edges()
{
  m_dijkstra.pop();

  ScanTaskPoint destination(0, m_start_current);
  m_dijkstra.link(destination, destination, 0);

  if (m_start_current<m_start_limit) {
    m_start_current++;
  } else {
    m_start_current = 0;
    m_start_limit = find_start_limit();
  }
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
