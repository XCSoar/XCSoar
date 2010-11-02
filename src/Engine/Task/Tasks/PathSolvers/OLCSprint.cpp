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
#include "OLCSprint.hpp"

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
