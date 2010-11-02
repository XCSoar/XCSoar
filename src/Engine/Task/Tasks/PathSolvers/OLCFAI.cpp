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
#include "OLCFAI.hpp"
#include "Navigation/Flat/FlatRay.hpp"

/*
 @todo potential to use 3d convex hull to speed search

 2nd point inclusion rules:
   if min leg length is 25%, max is 45%
   pmin = 0.25
   pmax = 0.45
   ptot = 2*pmin+pmax+0.05
   with 2 legs:
      ptot = pmin+pmax


  0: start
  1: first leg start
  2: second leg start
  3: third leg start
  4: end
*/

OLCFAI::OLCFAI(const Trace &_trace,
  const unsigned &_handicap):
  ContestDijkstra(_trace, _handicap, 3, 1000),
  is_closed(false),
  is_complete(false),
  first_tp(0) 
{}


void 
OLCFAI::reset()
{
  ContestDijkstra::reset();
  is_complete = false;
  is_closed = false;
  first_tp = 0;
  best_d = 0;
}


fixed
OLCFAI::leg_distance(const unsigned index) const
{  
  // leg 0: 1-2
  // leg 1: 2-3
  // leg 2: 3-1

  const GeoPoint &p_start = solution[index].get_location();
  const GeoPoint &p_dest = (index < 2)? 
    solution[index+1].get_location():
    solution[0].get_location();

  return p_start.distance(p_dest);
}


bool 
OLCFAI::path_closed() const
{

  // RESERVED FOR FUTURE USE: DO NOT DELETE

  // note this may fail if resolution of sampled trace is too low
  static const fixed fixed_1000(1000);
  const ScanTaskPoint end(0, n_points-1);
  fixed d_min(-1);
  unsigned i_min;

  ScanTaskPoint start(0, 0);

  for (start.second=0; start.second <= first_tp; 
       ++start.second) {

    const fixed d_this =
      get_point(start).get_location().distance(
        get_point(end).get_location());

    if (!positive(d_min) || (d_this < d_min)) {
      d_min = d_this;
      i_min = start.second;
    } 
    if (d_this<= fixed_1000) {
      return true;
    }
  }

//  std::cout << d_min << " " << i_min << "\n";

  return false;
}


unsigned 
OLCFAI::second_leg_distance(const ScanTaskPoint &destination,
  unsigned &best) const
{
  // this is a heuristic to remove invalid triangles
  // we do as much of this in flat projection for speed

  const unsigned df_1 = solution[0].flat_distance(solution[1]);
  const unsigned df_2 = solution[1].flat_distance(solution[2]);
  const unsigned df_3 = solution[2].flat_distance(solution[0]);
  const unsigned df_total = df_1+df_2+df_3;

  // require some distance!
  if (df_total<20) {
    return 0;
  }

  // no point scanning if worst than best
  if (df_total<= best) {
    return 0;
  }

  const unsigned shortest = min(df_1, min(df_2, df_3));

  // require all legs to have distance
  if (!shortest) {
    return 0;
  }
  if (shortest*4<df_total) { // fails min < 25% worst-case rule!
    return 0;
  }

  const unsigned d = df_3+df_2;

  if (shortest*25>=df_total*7) { 
    // passes min > 28% rule,
    // this automatically means we pass max > 45% worst-case
    best = df_total;
    return d;
  }

  const unsigned longest = max(df_1, max(df_2, df_3));
  if (longest*20>df_total*9) { // fails max > 45% worst-case rule!
    return 0;
  }

  // passed basic tests, now detailed ones

  // find accurate min leg distance
  fixed leg(0);
  if (df_1 == shortest) {
    leg = leg_distance(0);
  } else if (df_2 == shortest) {
    leg = leg_distance(1);
  } else if (df_3 == shortest) {
    leg = leg_distance(2);
  }

  // estimate total distance by scaling.
  // this is a slight approximation, but saves having to do
  // three accurate distance calculations.

  const fixed d_total((df_total*leg)/shortest);
  if (d_total>=fixed(500000)) {
    // long distance, ok that it failed 28% rule
    best = df_total;
    return d;
  }
  return 0;
}


void
OLCFAI::add_start_edges()
{
  // use last point as single start,
  // this is out of order but required

  m_dijkstra.pop();
  ScanTaskPoint destination(0, n_points-1);
  m_dijkstra.link(destination, destination, 0);
}


void 
OLCFAI::add_edges(DijkstraTaskPoint &dijkstra, const ScanTaskPoint& origin)
{
  ScanTaskPoint destination(origin.first+1, origin.second+1);

  switch (destination.first) {
  case 1:
    // add points up to finish
    for (destination.second=0; destination.second < origin.second; 
         ++destination.second) {
      const unsigned d = 
        distance(origin, destination);

      if (4*d >= best_d) { // no reason to add candidate if worse than 25% rule
        dijkstra.link(destination, origin, get_weighting(origin.first)*d);
      }

    }
    break;
  case 2:
    find_solution(dijkstra, origin);

    // give first leg points to penultimate node
    for (; destination.second < n_points-1; ++destination.second) {
      solution[2] = get_point(destination);
      const unsigned d = second_leg_distance(destination, best_d);
      if (d) {
        dijkstra.link(destination, origin, get_weighting(origin.first)*d);

        // we have an improved solution
        is_complete = true;

        // need to scan again whether path is closed
        is_closed = false;
        first_tp = origin.second;
      }
    }
    break;
  case 3:
    // dummy just to close the triangle
    destination.second = n_points-1;
    dijkstra.link(destination, origin, 0);
    break;
  default:
    assert(1);
  }
}


fixed
OLCFAI::calc_distance() const
{
  if (is_complete) {
    return leg_distance(0)+leg_distance(1)+leg_distance(2);
  } else {
    return fixed_zero;
  }
}


fixed
OLCFAI::calc_time() const
{
  const ScanTaskPoint start(0, 0);
  const ScanTaskPoint end(0, n_points-1);
  return fixed(get_point(end).time - get_point(start).time);
}


fixed
OLCFAI::calc_score() const
{
  return apply_handicap(calc_distance()*fixed(0.0003));
}

void
OLCFAI::start_search()
{
}


bool 
OLCFAI::update_score()
{
  // RESERVED FOR FUTURE USE: DO NOT DELETE
  /*
  if (positive(get_best_score()) && !is_closed) {
    if (path_closed()) {
      std::cout << "complete, closed\n";
      is_closed = true;
    } else {
      std::cout << "complete, not closed\n";
      return true;
    }
  }
  */
  return false;
}
