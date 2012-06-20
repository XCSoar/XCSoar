/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
                         const bool _is_fai)
  :ContestDijkstra(_trace, false, 3, 1000),
   is_fai(_is_fai),
   is_closed(false),
   is_complete(false),
   first_tp(0)
{
}

void
OLCTriangle::Reset()
{
  ContestDijkstra::Reset();
  is_complete = false;
  is_closed = false;
  first_tp = 0;
  best_d = 0;
}

fixed
OLCTriangle::CalcLegDistance(const unsigned index) const
{
  // leg 0: 1-2
  // leg 1: 2-3
  // leg 2: 3-1

  const GeoPoint &p_start = GetPoint(solution[index]).get_location();
  const GeoPoint &p_dest =
    GetPoint(solution[index < 2 ? index + 1 : 0]).get_location();

  return p_start.Distance(p_dest);
}

bool
OLCTriangle::IsPathClosed() const
{

  // RESERVED FOR FUTURE USE: DO NOT DELETE

  // note this may fail if resolution of sampled trace is too low
  assert(n_points > 0);

  const GeoPoint end_location = GetPoint(n_points - 1).get_location();

  fixed d_min(-1);

  assert(first_tp < n_points);

  for (unsigned start_index = 0; start_index <= first_tp; ++start_index) {
    const fixed d_this =
      GetPoint(start_index).get_location().Distance(end_location);

    if (!positive(d_min) || d_this < d_min)
      d_min = d_this;

    if (d_this<= fixed_int_constant(1000))
      return true;
  }

  return false;
}

class TriangleSecondLeg {
  const bool is_fai;
  const SearchPoint a, b;
  const unsigned df_1;

public:
  TriangleSecondLeg(bool _fai, const SearchPoint &_a, const SearchPoint &_b)
    :is_fai(_fai), a(_a), b(_b), df_1(a.flat_distance(b)) {}

  struct Result {
    unsigned leg_distance, total_distance;

    Result(unsigned _leg, unsigned _total)
      :leg_distance(_leg), total_distance(_total) {}
  };

  gcc_pure
  Result Calculate(const SearchPoint &c, unsigned best) const;
};

TriangleSecondLeg::Result
TriangleSecondLeg::Calculate(const SearchPoint &c, unsigned best) const
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
  if (!shortest)
    return Result(0, 0);

  if (is_fai && (shortest*4<df_total))
    // fails min < 25% worst-case rule!
    return Result(0, 0);

  const unsigned d = df_3 + df_2;

  // without FAI rules, allow any triangle
  if (!is_fai)
    return Result(d, df_total);

  if (shortest * 25 >= df_total * 7)
    // passes min > 28% rule,
    // this automatically means we pass max > 45% worst-case
    return Result(d, df_total);

  const unsigned longest = max(df_1, max(df_2, df_3));
  if (longest * 20 > df_total * 9) // fails max > 45% worst-case rule!
    return Result(0, 0);

  // passed basic tests, now detailed ones

  // find accurate min leg distance
  fixed leg(0);
  if (df_1 == shortest)
    leg = a.get_location().Distance(b.get_location());
  else if (df_2 == shortest)
    leg = b.get_location().Distance(c.get_location());
  else if (df_3 == shortest)
    leg = c.get_location().Distance(a.get_location());

  // estimate total distance by scaling.
  // this is a slight approximation, but saves having to do
  // three accurate distance calculations.

  const fixed d_total(df_total* leg / shortest);
  if (d_total >= fixed(500000)) {
    // long distance, ok that it failed 28% rule
    return Result(d, df_total);
  }

  return Result(0, 0);
}


void
OLCTriangle::AddStartEdges()
{
  // use last point as single start,
  // this is out of order but required

  ScanTaskPoint destination(0, n_points - 1);
  LinkStart(destination);
}

void
OLCTriangle::AddEdges(const ScanTaskPoint origin)
{
  assert(origin.GetPointIndex() < n_points);

  switch (origin.GetStageNumber()) {
  case 0:
    // add points up to finish
    for (ScanTaskPoint destination(origin.GetStageNumber() + 1, 0),
           end(origin.GetStageNumber() + 1, origin.GetPointIndex());
         destination != end; destination.IncrementPointIndex()) {
      const unsigned d = CalcEdgeDistance(origin, destination);

      if (!is_fai || 4 * d >= best_d)
        /* no reason to add candidate if worse than 25% rule for FAI
           tasks */
        Link(destination, origin, GetStageWeight(origin.GetStageNumber()) * d);
    }
    break;

  case 1: {
    ScanTaskPoint previous = dijkstra.GetPredecessor(origin);

    // give first leg points to penultimate node
    TriangleSecondLeg sl(is_fai, GetPoint(previous), GetPoint(origin));
    for (ScanTaskPoint destination(origin.GetStageNumber() + 1,
                                   origin.GetPointIndex() + 1),
           end(origin.GetStageNumber() + 1, n_points - 1);
         destination != end; destination.IncrementPointIndex()) {
      TriangleSecondLeg::Result result = sl.Calculate(GetPoint(destination),
                                                      best_d);
      const unsigned d = result.leg_distance;
      if (d > 0) {
        best_d = result.total_distance;

        Link(destination, origin, GetStageWeight(origin.GetStageNumber()) * d);

        // we have an improved solution
        is_complete = true;

        // need to scan again whether path is closed
        is_closed = false;
        first_tp = origin.GetPointIndex();
      }
    }
  }
    break;

  case 2:
    // dummy just to close the triangle
    Link(ScanTaskPoint(origin.GetStageNumber() + 1, n_points - 1), origin, 0);
    break;

  default:
    assert(false);
  }
}

fixed
OLCTriangle::CalcDistance() const
{
  if (is_complete) {
    return CalcLegDistance(0) + CalcLegDistance(1) + CalcLegDistance(2);
  } else {
    return fixed_zero;
  }
}

fixed
OLCTriangle::CalcTime() const
{
  if (!n_points)
    return fixed_zero;

  return fixed(GetPoint(n_points - 1).DeltaTime(GetPoint(0)));
}

fixed
OLCTriangle::CalcScore() const
{
  // one point per km
  return ApplyHandicap(CalcDistance()*fixed(0.001));
}

void
OLCTriangle::StartSearch()
{
}

bool
OLCTriangle::UpdateScore()
{
  // RESERVED FOR FUTURE USE: DO NOT DELETE
  /*
  if (positive(get_best_score()) && !is_closed) {
    if (IsPathClosed()) {
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
OLCTriangle::SaveSolution()
{
  assert(num_stages <= MAX_STAGES);
  assert(solution_valid);

  if (AbstractContest::SaveSolution()) {
    best_solution.clear();

    for (int i = 3; i >= 0; --i)
      best_solution.append(GetPoint(solution[i]));

    return true;
  }

  return false;
}
