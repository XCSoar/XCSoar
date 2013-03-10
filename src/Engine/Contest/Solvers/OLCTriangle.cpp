/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Cast.hpp"
#include "Trace/Trace.hpp"

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

/**
 * Maximum allowed distance between start end finish.  According to
 * FAI-OLC 2012 rules, this is 1 km.
 *
 * TODO: due to trace thinning, our TracePoints are sometimes not
 * close enough for this check to succeed.  To work around this for
 * now, we allow up to 5 km until this library has been implemented
 * properly.
 */
static constexpr fixed max_distance(5000);

OLCTriangle::OLCTriangle(const Trace &_trace,
                         const bool _is_fai, bool _predict)
  :ContestDijkstra(_trace, false, 4, 1000),
   is_fai(_is_fai), predict(_predict),
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

gcc_pure
static fixed
CalcLegDistance(const ContestTraceVector &solution, const unsigned index)
{
  // leg 0: 1-2
  // leg 1: 2-3
  // leg 2: 3-1

  const GeoPoint &p_start = solution[index + 1].GetLocation();
  const GeoPoint &p_dest = solution[index < 2 ? index + 2 : 1].GetLocation();

  return p_start.Distance(p_dest);
}

bool
OLCTriangle::IsPathClosed() const
{

  // RESERVED FOR FUTURE USE: DO NOT DELETE

  // note this may fail if resolution of sampled trace is too low
  assert(n_points > 0);

  const GeoPoint end_location = GetPoint(n_points - 1).GetLocation();

  fixed d_min(-1);

  assert(first_tp < n_points);

  for (unsigned start_index = 0; start_index <= first_tp; ++start_index) {
    const fixed d_this =
      GetPoint(start_index).GetLocation().Distance(end_location);

    if (!positive(d_min) || d_this < d_min)
      d_min = d_this;

    if (d_this <= max_distance)
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
    :is_fai(_fai), a(_a), b(_b), df_1(a.FlatDistanceTo(b)) {}

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

  const unsigned df_2 = b.FlatDistanceTo(c);
  const unsigned df_3 = c.FlatDistanceTo(a);
  const unsigned df_total = df_1+df_2+df_3;

  // require some distance!
  if (df_total<20) {
    return Result(0, 0);
  }

  // no point scanning if worst than best
  if (df_total<= best) {
    return Result(0, 0);
  }

  const unsigned shortest = std::min({df_1, df_2, df_3});

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

  const unsigned longest = std::max({df_1, df_2, df_3});
  if (longest * 20 > df_total * 9) // fails max > 45% worst-case rule!
    return Result(0, 0);

  // passed basic tests, now detailed ones

  // find accurate min leg distance
  fixed leg(0);
  if (df_1 == shortest)
    leg = a.GetLocation().Distance(b.GetLocation());
  else if (df_2 == shortest)
    leg = b.GetLocation().Distance(c.GetLocation());
  else if (df_3 == shortest)
    leg = c.GetLocation().Distance(a.GetLocation());

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
OLCTriangle::AddTurn0Edges(const ScanTaskPoint origin)
{
  // add points up to finish

  const ScanTaskPoint begin(origin.GetStageNumber() + 1, 0);
  const ScanTaskPoint end(origin.GetStageNumber() + 1, origin.GetPointIndex());

  for (ScanTaskPoint i = begin; i != end; i.IncrementPointIndex()) {
    // Edge cost is 0, because this edge just moves the first TP.
    Link(i, origin, 0);
  }
}


void
OLCTriangle::AddTurn1Edges(const ScanTaskPoint origin)
{
  // add points up to finish

  const ScanTaskPoint begin(origin.GetStageNumber() + 1, 0);
  const ScanTaskPoint end(origin.GetStageNumber() + 1, origin.GetPointIndex());

  for (ScanTaskPoint i = begin; i != end; i.IncrementPointIndex()) {
    const unsigned d = CalcEdgeDistance(origin, i);

    if (!is_fai || 4 * d >= best_d)
      /* no reason to add candidate if worse than 25% rule for FAI
         tasks */
      Link(i, origin, GetStageWeight(origin.GetStageNumber()) * d);
  }
}

void
OLCTriangle::AddTurn2Edges(const ScanTaskPoint origin)
{
  ScanTaskPoint previous = dijkstra.GetPredecessor(origin);

  // give first leg points to penultimate node
  TriangleSecondLeg sl(is_fai, GetPoint(previous), GetPoint(origin));

  const ScanTaskPoint begin(origin.GetStageNumber() + 1, 0);
  const ScanTaskPoint end(origin.GetStageNumber() + 1,
                          origin.GetPointIndex());

  for (ScanTaskPoint i = begin; i != end; i.IncrementPointIndex()) {
    TriangleSecondLeg::Result result = sl.Calculate(GetPoint(i), best_d);
    const unsigned d = result.leg_distance;
    if (d > 0) {
      best_d = result.total_distance;

      Link(i, origin, GetStageWeight(origin.GetStageNumber()) * d);

      // we have an improved solution
      is_complete = true;

      // need to scan again whether path is closed
      is_closed = false;
      first_tp = origin.GetPointIndex();
    }
  }
}

void
OLCTriangle::AddFinishEdges(const ScanTaskPoint origin)
{
  assert(IsFinal(origin.GetStageNumber() + 1));

  if (predict) {
    // dummy just to close the triangle
    is_closed = true;
    Link(ScanTaskPoint(origin.GetStageNumber() + 1, n_points - 1), origin, 0);
  } else {
    const ScanTaskPoint start = FindStart(origin);
    const ScanTaskPoint tp1 = FindStage(origin, 1);
    const TracePoint &start_point = GetPoint(start);
    const TracePoint &origin_point = GetPoint(origin);

    const unsigned max_range =
      trace_master.ProjectRange(origin_point.GetLocation(), max_distance);

    /* check all remaining points, see which ones match the
       conditions */
    const ScanTaskPoint begin(origin.GetStageNumber() + 1, 0);
    const ScanTaskPoint end(origin.GetStageNumber() + 1,
                            origin.GetPointIndex());
    for (ScanTaskPoint i = begin; i != end; i.IncrementPointIndex())
      if (CalcEdgeDistance(start, i) <= max_range &&
          start_point.GetLocation().Distance(GetPoint(i).GetLocation()) < max_distance) {
        is_closed = true;
        // use the cost of tp3 to tp1, omitting the start/end
        Link(i, origin, GetStageWeight(origin.GetStageNumber()) * CalcEdgeDistance(origin, tp1));
      }
  }
}

void
OLCTriangle::AddEdges(const ScanTaskPoint origin)
{
  assert(origin.GetPointIndex() < n_points);

  switch (origin.GetStageNumber()) {
  case 0:
    AddTurn0Edges(origin);
    break;

  case 1:
    AddTurn1Edges(origin);
    break;

  case 2:
    AddTurn2Edges(origin);
    break;

  case 3:
    AddFinishEdges(origin);
    break;

  default:
    gcc_unreachable();
  }
}

ContestResult
OLCTriangle::CalculateResult(const ContestTraceVector &solution) const
{
  ContestResult result;
  result.time = n_points > 0
    ? fixed(GetPoint(n_points - 1).DeltaTime(GetPoint(0)))
    : fixed(0);
  result.distance = (is_complete && is_closed)
    ? CalcLegDistance(solution, 0) + CalcLegDistance(solution, 1) + CalcLegDistance(solution, 2)
    : fixed(0);
  result.score = ApplyHandicap(result.distance * fixed(0.001));
  return result;
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

void
OLCTriangle::CopySolution(ContestTraceVector &result) const
{
  assert(num_stages <= MAX_STAGES);

  ContestDijkstra::CopySolution(result);
  assert(result.size() == 5);

  std::reverse(result.begin(), result.end());
}
