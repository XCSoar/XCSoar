/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "OLCTriangle.hpp"
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

OLCTriangle::OLCTriangle(const Trace &_trace,
                         const unsigned &_handicap,
                         const bool _is_fai):
  ContestDijkstra(_trace, _handicap, 3, 1000),
  is_closed(false),
  is_complete(false),
  first_tp(0),
  is_fai(_is_fai)
{}


void 
OLCTriangle::reset()
{
  ContestDijkstra::reset();
  is_complete = false;
  is_closed = false;
  first_tp = 0;
  best_d = 0;
}


fixed
OLCTriangle::leg_distance(const unsigned index) const
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
OLCTriangle::path_closed() const
{

  // RESERVED FOR FUTURE USE: DO NOT DELETE

  // note this may fail if resolution of sampled trace is too low
  assert(n_points > 0);
  const ScanTaskPoint end(0, n_points-1);
  fixed d_min(-1);

  ScanTaskPoint start(0, 0);

  assert(first_tp < n_points);
  for (start.point_index = 0; start.point_index <= first_tp;
       ++start.point_index) {

    const fixed d_this =
      GetPointFast(start).get_location().distance(
        GetPointFast(end).get_location());

    if (!positive(d_min) || (d_this < d_min)) {
      d_min = d_this;
    } 
    if (d_this<= fixed_int_constant(1000)) {
      return true;
    }
  }

  return false;
}

class TriangleSecondLeg {
  const bool is_fai;
  const TracePoint a, b;
  const unsigned df_1;

public:
  TriangleSecondLeg(bool _fai, const TracePoint &_a, const TracePoint &_b)
    :is_fai(_fai), a(_a), b(_b), df_1(a.flat_distance(b)) {}

  struct Result {
    unsigned leg_distance, total_distance;

    Result(unsigned _leg, unsigned _total)
      :leg_distance(_leg), total_distance(_total) {}
  };

  gcc_pure
  Result Calculate(const TracePoint &c, unsigned best) const;
};

TriangleSecondLeg::Result
TriangleSecondLeg::Calculate(const TracePoint &c, unsigned best) const
{
  // this is a heuristic to remove invalid triangles
  // we do as much of this in flat projection for speed

  const unsigned df_2 = b.flat_distance(c);
  const unsigned df_3 = c.flat_distance(a);
  const unsigned df_total = df_1+df_2+df_3;

  // require some distance!
  if (df_total<20) {
    return Result(0, 0);
  }

  // no point scanning if worst than best
  if (df_total<= best) {
    return Result(0, 0);
  }

  const unsigned shortest = min(df_1, min(df_2, df_3));

  // require all legs to have distance
  if (!shortest) {
    return Result(0, 0);
  }

  if (is_fai && (shortest*4<df_total)) { // fails min < 25% worst-case rule!
    return Result(0, 0);
  }

  const unsigned d = df_3+df_2;

  // without FAI rules, allow any triangle
  if (!is_fai) {
    return Result(d, df_total);
  }

  if (shortest*25>=df_total*7) { 
    // passes min > 28% rule,
    // this automatically means we pass max > 45% worst-case
    return Result(d, df_total);
  }

  const unsigned longest = max(df_1, max(df_2, df_3));
  if (longest*20>df_total*9) { // fails max > 45% worst-case rule!
    return Result(0, 0);
  }

  // passed basic tests, now detailed ones

  // find accurate min leg distance
  fixed leg(0);
  if (df_1 == shortest) {
    leg = a.get_location().distance(b.get_location());
  } else if (df_2 == shortest) {
    leg = b.get_location().distance(c.get_location());
  } else if (df_3 == shortest) {
    leg = c.get_location().distance(a.get_location());
  }

  // estimate total distance by scaling.
  // this is a slight approximation, but saves having to do
  // three accurate distance calculations.

  const fixed d_total((df_total*leg)/shortest);
  if (d_total>=fixed(500000)) {
    // long distance, ok that it failed 28% rule
    return Result(d, df_total);
  }

  return Result(0, 0);
}


void
OLCTriangle::add_start_edges()
{
  // use last point as single start,
  // this is out of order but required

  dijkstra.pop();
  ScanTaskPoint destination(0, n_points-1);
  dijkstra.link(destination, destination, 0);
}


void 
OLCTriangle::add_edges(const ScanTaskPoint& origin)
{
  assert(origin.point_index < n_points);
  ScanTaskPoint destination(origin.stage_number + 1, origin.point_index + 1);

  switch (destination.stage_number) {
  case 1:
    // add points up to finish
    for (destination.point_index = 0;
         destination.point_index < origin.point_index;
         ++destination.point_index) {
      const unsigned d = 
        distance(origin, destination);

      if (!is_fai || (4*d >= best_d)) { // no reason to add candidate if worse
                                        // than 25% rule for FAI tasks
        dijkstra.link(destination, origin,
                      get_weighting(origin.stage_number) * d);
      }

    }
    break;
  case 2: {
    find_solution(origin);

    // give first leg points to penultimate node
    TriangleSecondLeg sl(is_fai, solution[0], solution[1]);
    for (; destination.point_index < n_points-1; ++destination.point_index) {
      TriangleSecondLeg::Result result = sl.Calculate(GetPointFast(destination),
                                                      best_d);
      const unsigned d = result.leg_distance;
      if (d) {
        best_d = result.total_distance;

        dijkstra.link(destination, origin,
                      get_weighting(origin.stage_number) * d);

        // we have an improved solution
        is_complete = true;

        // need to scan again whether path is closed
        is_closed = false;
        first_tp = origin.point_index;
      }
    }
  }
    break;
  case 3:
    // dummy just to close the triangle
    destination.point_index = n_points - 1;
    dijkstra.link(destination, origin, 0);
    break;
  default:
    assert(1);
  }
}


fixed
OLCTriangle::calc_distance() const
{
  if (is_complete) {
    return leg_distance(0)+leg_distance(1)+leg_distance(2);
  } else {
    return fixed_zero;
  }
}


fixed
OLCTriangle::calc_time() const
{
  if (!n_points)
    return fixed_zero;

  const ScanTaskPoint start(0, 0);
  const ScanTaskPoint end(0, n_points-1);
  return fixed(GetPointFast(end).time - GetPointFast(start).time);
}


fixed
OLCTriangle::calc_score() const
{
  // one point per km
  return apply_handicap(calc_distance()*fixed(0.001));
}

void
OLCTriangle::start_search()
{
}


bool 
OLCTriangle::update_score()
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

bool
OLCTriangle::save_solution()
{
  assert(num_stages <= MAX_STAGES);

  if (AbstractContest::save_solution()) {
    best_solution.clear();

    best_solution.append(solution[3]);
    best_solution.append(solution[1]);
    best_solution.append(solution[2]);
    best_solution.append(solution[0]);

    return true;
  }
  return false;
}
