#include "OLCClassic.hpp"

OLCClassic::OLCClassic(const TracePointVector &_trace):
  ContestDijkstra(_trace, 6, 1000) {}

void 
OLCClassic::set_weightings()
{
  m_weightings[0] = 5;
  m_weightings[1] = 5;
  m_weightings[2] = 5;
  m_weightings[3] = 5;
  m_weightings[4] = 5; // was 4, pre 2010 rules
  m_weightings[5] = 5; // was 3, pre 2010 rules
}
