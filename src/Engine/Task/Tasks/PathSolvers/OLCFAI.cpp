#include "OLCFAI.hpp"

OLCFAI::OLCFAI(OnlineContest& _olc):
  OLCDijkstra(_olc, 2, 3000) 
{

}


bool 
OLCFAI::finish_satisfied(const ScanTaskPoint &sp) const
{
  /// \todo implement checks for distance constraints
  // if d<500km, shortest leg >= 28% d; else shortest leg >= 25%d

  return OLCDijkstra::finish_satisfied(sp);
}


bool 
OLCFAI::admit_candidate(const ScanTaskPoint &candidate) const
{
  /// \todo implement check for closure
  /// (end point is within 1km of start)
  return OLCDijkstra::admit_candidate(candidate);
}
