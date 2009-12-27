/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "OLCDijkstra.hpp"
#include "Dijkstra.hpp"
#include "Task/Tasks/OnlineContest.hpp"

OLCDijkstra::~OLCDijkstra() {
}


OLCDijkstra::OLCDijkstra(OnlineContest& _olc, const unsigned n_legs):
  NavDijkstra(n_legs+1),
  olc(_olc),
  n_points(_olc.get_sample_points().size())
{
  m_weightings.reserve(n_legs);
}

void
OLCDijkstra::set_weightings()
{
  for (unsigned i=0; i+1<num_stages; ++i) {
    m_weightings[i] = 1;
  }
}

const SearchPoint &
OLCDijkstra::get_point(const ScanTaskPoint &sp) const
{
  return olc.get_sample_points()[sp.second];
}


unsigned 
OLCDijkstra::solve()
{
  if (n_points<num_stages) { // todo check this
    return 0;
  }

  shortest = false;

  set_weightings();

  const ScanTaskPoint start(0,0);
  DijkstraTaskPoint dijkstra(start);

  add_start_edges(dijkstra);

  const unsigned d= distance_general(dijkstra);
  return d;
}

void 
OLCDijkstra::add_edges(DijkstraTaskPoint &dijkstra,
                       const ScanTaskPoint& origin) 
{
  ScanTaskPoint destination;
  destination.first = origin.first+1;
  destination.second = origin.second+1;
  const unsigned end = (int)n_points+origin.first-num_stages+1;
  
  find_solution(dijkstra, destination);
  
  for (; destination.second< end; ++destination.second) {
    dijkstra.link(destination, origin, weighted_distance(origin, destination));
  }
}

void
OLCDijkstra::add_start_edges(DijkstraTaskPoint &dijkstra)
{

  ScanTaskPoint destination;
  destination.first = 0;
  destination.second = 0;
  const unsigned end = (int)n_points-num_stages+1;

  dijkstra.pop();

  for (; destination.second< end; ++destination.second) {
    dijkstra.link(destination, destination, 0);
  }
}

unsigned 
OLCDijkstra::weighted_distance(const ScanTaskPoint &sp1,
                               const ScanTaskPoint &sp2) const
{
  return m_weightings[sp1.first]*distance(sp1, sp2);
}


bool 
OLCDijkstra::finish_satisfied() const
{
  /// \todo implement checks for some OLC types
  /// - e.g. triangle leg
  return true;
}
