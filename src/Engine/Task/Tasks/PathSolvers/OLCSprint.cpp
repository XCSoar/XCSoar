#include "OLCSprint.hpp"


OLCSprint::OLCSprint(OnlineContest& _olc):
  OLCDijkstra(_olc, 4, 0) 
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
