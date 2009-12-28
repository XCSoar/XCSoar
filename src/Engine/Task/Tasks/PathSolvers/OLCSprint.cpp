#include "OLCSprint.hpp"

OLCSprint::OLCSprint(OnlineContest& _olc):
  OLCDijkstra(_olc, 5, 0) 
{

}

bool 
OLCSprint::admit_candidate(const ScanTaskPoint &candidate) const
{
  return (get_point(candidate).time <=
          solution[0].time + 9000) &&
    OLCDijkstra::admit_candidate(candidate);
}

