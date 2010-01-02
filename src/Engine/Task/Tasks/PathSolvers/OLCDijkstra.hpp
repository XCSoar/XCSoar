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

#ifndef OLC_DIJKSTRA_HPP
#define OLC_DIJKSTRA_HPP

#include "NavDijkstra.hpp"
#include "Navigation/TracePoint.hpp"
#include "Math/fixed.hpp"

class OnlineContest;

/**
 * @todo This currently only finds the first solution, not the best solution;
 *       needs a little more work
 *
 */
class OLCDijkstra: 
  public NavDijkstra<TracePoint>
{
public:
/**
 * Constructor
 *
 * @param _olc The OLC task to solve for
 * @param n_legs Maximum number of legs in OLC task
 * @param finish_alt_diff Maximum height loss from start to finish (m)
 */
  OLCDijkstra(OnlineContest& _olc, 
              const unsigned n_legs,
              const unsigned finish_alt_diff = 3000);

/**
 * Destructor, frees local variables
 */
  ~OLCDijkstra();

  virtual fixed score(fixed &the_distance);

  void copy_solution(TracePointVector &vec);

  fixed calc_distance() const;
  fixed calc_time() const;

  void reset();

  bool solve();

protected:
  const TracePoint &get_point(const ScanTaskPoint &sp) const;

  virtual void add_edges(DijkstraTaskPoint &dijkstra,
                         const ScanTaskPoint &curNode);

  virtual bool finish_satisfied(const ScanTaskPoint &sp) const;

  virtual void set_weightings();

  virtual bool admit_candidate(const ScanTaskPoint &candidate) const;

  std::vector<unsigned> m_weightings;

  void set_rank(const ScanTaskPoint &sp, const unsigned d);

  unsigned dist_to_rank(const unsigned dist) const;

  bool m_reverse;

private:

  unsigned get_weighting(const unsigned i) const;

  OnlineContest& olc;
  unsigned n_points;
  const unsigned m_finish_alt_diff;

  bool solution_found;

  void add_start_edges();

  DijkstraTaskPoint m_dijkstra;

  bool m_rank_mode;
};

#endif
