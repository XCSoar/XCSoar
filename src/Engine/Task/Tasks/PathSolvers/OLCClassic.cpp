#include "OLCClassic.hpp"

OLCClassic::OLCClassic(OnlineContest& _olc):
  OLCDijkstra(_olc, 6, 3000) 
{

}

void 
OLCClassic::set_weightings()
{
  m_weightings.clear();
  m_weightings.push_back(5);
  m_weightings.push_back(5);
  m_weightings.push_back(5);
  m_weightings.push_back(4);
  m_weightings.push_back(4);
  m_weightings.push_back(3);
}
