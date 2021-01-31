/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "TriangleContest.hpp"
#include "Cast.hpp"
#include "Trace/Trace.hpp"
#include "util/QuadTree.hxx"

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
static constexpr double max_distance(1000);

TriangleContest::TriangleContest(const Trace &_trace,
                                 bool _predict,
                                 const unsigned _finish_alt_diff) noexcept
  : AbstractContest(_finish_alt_diff),
   TraceManager(_trace),
   predict(_predict)
{
}

void
TriangleContest::Reset() noexcept
{
  is_complete = false;
  is_closed = false;
  best_d = 0;

  // set tick_iterations to a default value,
  // this should be adjusted when the trace size is known
  tick_iterations = 1000;

  closing_pairs.Clear();
  ClearTrace();

  ResetBranchAndBound();
  AbstractContest::Reset();
}

void
TriangleContest::ResetBranchAndBound() noexcept
{
  running = false;
  branch_and_bound.clear();
}

gcc_pure
static double
CalcLegDistance(const ContestTraceVector &solution,
                const unsigned index) noexcept
{
  // leg 0: 1-2
  // leg 1: 2-3
  // leg 2: 3-1

  const GeoPoint &p_start = solution[index + 1].GetLocation();
  const GeoPoint &p_dest = solution[index < 2 ? index + 2 : 1].GetLocation();

  return p_start.Distance(p_dest);
}

void
TriangleContest::UpdateTrace(bool force) noexcept
{
  if (IsMasterAppended()) return; /* unmodified */

  if (force || IsMasterUpdated(false)) {
    UpdateTraceFull();

    is_complete = false;

    best_d = 0;

    closing_pairs.Clear();
    is_closed = FindClosingPairs(0);

   } else if (is_complete && incremental) {
    const unsigned old_size = n_points;
    if (UpdateTraceTail()) {
      is_complete = false;
      is_closed = FindClosingPairs(old_size);
    }
  }

  /**
   * Update tick_iterations to a sensible value. Statistical analysis
   * revealed that the branch and bound algorithm worst running time
   * is around O(n_points^2.2), it's best can be better than O(n_points).
   * Using n_points^2 / 8 as number of iterations per tick should allow
   * the algorithm to finish in about 10 to 15 ticks in most cases.
   */
  tick_iterations = n_points * n_points / 8;
}

SolverResult
TriangleContest::Solve(bool exhaustive) noexcept
{
  if (trace_master.size() < 3) {
    ClearTrace();
    is_complete = false;
    return SolverResult::FAILED;
  }

  if (!running) {
    // branch and bound is currently in finished state, update trace
    UpdateTrace(exhaustive);
  }

  if (!is_complete || running) {
    if (n_points < 3) {
      ResetBranchAndBound();
      return SolverResult::FAILED;
    }

    if (is_closed)
      SolveTriangle(exhaustive);

    if (!SaveSolution())
      return SolverResult::FAILED;

    return SolverResult::VALID;
  } else {
    return SolverResult::FAILED;
  }
}

void
TriangleContest::SolveTriangle(bool exhaustive) noexcept
{
  unsigned tp1 = 0,
           tp2 = 0,
           tp3 = 0,
           start = 0,
           finish = 0;

  if (exhaustive || !predict) {
    ClosingPairs relaxed_pairs;

    unsigned relax = n_points * 0.03;

    // for all closed trace loops
    for (auto closing_pair = closing_pairs.closing_pairs.begin();
         closing_pair != closing_pairs.closing_pairs.end();
         ++closing_pair) {

      auto already_relaxed = relaxed_pairs.FindRange(*closing_pair);
      if (already_relaxed.first != 0 || already_relaxed.second != 0)
        // this pair is already relaxed... continue with next
        continue;

      unsigned relax_first = closing_pair->first;
      unsigned relax_last = closing_pair->second;

      const unsigned max_first = closing_pair->first + relax;
      const unsigned max_last = closing_pair->second + relax;

      for (auto relaxed = std::next(closing_pair);
           relaxed != closing_pairs.closing_pairs.end() &&
           relaxed->first <= max_first && relaxed->second <= max_last;
           ++relaxed)
        relax_last = std::max(relax_last, relaxed->second);

      relaxed_pairs.Insert(ClosingPair(relax_first, relax_last));
    }

    // TODO: reverse sort relaxed pairs according to number of contained points

    ClosingPairs close_look;

    for (const auto relaxed_pair : relaxed_pairs.closing_pairs) {

      const auto triangle = RunBranchAndBound(relaxed_pair.first,
                                              relaxed_pair.second,
                                              best_d, exhaustive);

      if (std::get<3>(triangle) > best_d) {
        // solution is better than best_d
        // only if triangle is inside a unrelaxed pair...

        auto unrelaxed = closing_pairs.FindRange(ClosingPair(std::get<0>(triangle), std::get<2>(triangle)));
        if (unrelaxed.first != 0 || unrelaxed.second != 0) {
          // fortunately it is inside a unrelaxed closing pair :-)
          start = unrelaxed.first;
          tp1 = std::get<0>(triangle);
          tp2 = std::get<1>(triangle);
          tp3 = std::get<2>(triangle);
          finish = unrelaxed.second;

          best_d = std::get<3>(triangle);
        } else {
          // otherwise we should solve the triangle again for every unrelaxed pair
          // contained inside the current relaxed pair. *damn!*
          for (const auto closing_pair : closing_pairs.closing_pairs) {
            if (closing_pair.first >= relaxed_pair.first &&
                closing_pair.second <= relaxed_pair.second)
              close_look.Insert(closing_pair);
         }
       }
      }
    }

    for (const auto &close_look_pair : close_look.closing_pairs) {
      const auto triangle = RunBranchAndBound(close_look_pair.first,
                                              close_look_pair.second,
                                              best_d, exhaustive);

      if (std::get<3>(triangle) > best_d) {
        // solution is better than best_d

        start = close_look_pair.first;
        tp1 = std::get<0>(triangle);
        tp2 = std::get<1>(triangle);
        tp3 = std::get<2>(triangle);
        finish = close_look_pair.second;

        best_d = std::get<3>(triangle);
      }
    }

  } else {
    /**
     * We're currently running in predictive, non-exhaustive mode, so we use
     * one closing pair only (0 -> n_points-1) which allows us to suspend the
     * solver...
     */
    const auto triangle = RunBranchAndBound(0, n_points - 1, best_d, false);

    if (std::get<3>(triangle) > best_d) {
      // solution is better than best_d

      start = 0;
      tp1 = std::get<0>(triangle);
      tp2 = std::get<1>(triangle);
      tp3 = std::get<2>(triangle);
      finish = n_points - 1;

      best_d = std::get<3>(triangle);
    }
  }

  if (best_d > 0) {
    solution.resize(5);

    solution[0] = TraceManager::GetPoint(start);
    solution[1] = TraceManager::GetPoint(tp1);
    solution[2] = TraceManager::GetPoint(tp2);
    solution[3] = TraceManager::GetPoint(tp3);
    solution[4] = TraceManager::GetPoint(finish);

    is_complete = true;
  }
}


std::tuple<unsigned, unsigned, unsigned, unsigned>
TriangleContest::RunBranchAndBound(unsigned from, unsigned to, unsigned worst_d,
                                   bool exhaustive) noexcept
{
  /* Some general information about the branch and bound method can be found here:
   * http://eaton.math.rpi.edu/faculty/Mitchell/papers/leeejem.html
   *
   * How to use this method for solving FAI triangles is described here:
   * http://www.penguin.cz/~ondrap/algorithm.pdf
   */

  // Return early if this tp-range can't beat the current best_d...
  // Assume a maximum speed of 100 m/s
  const unsigned fastskiprange = GetPoint(to).DeltaTime(GetPoint(from)) * 100;
  const unsigned fastskiprange_flat =
    trace_master.ProjectRange(GetPoint(from).GetLocation(), fastskiprange);

  if (fastskiprange_flat < worst_d)
    return {0, 0, 0, 0};

  bool integral_feasible = false;
  unsigned best_d = 0,
           tp1 = 0,
           tp2 = 0,
           tp3 = 0;
  unsigned iterations = 0;

  // note: this is _not_ the breakepoint between small and large triangles,
  // but a slightly lower value used for relaxed large triangle checking.
  const auto validator =
    OLCTriangleRules::MakeValidator(trace_master.GetProjection(),
                                    GetPoint(from).GetLocation());

  if (!running) {
    // initiate algorithm. otherwise continue unfinished run
    running = true;

    // initialize bound-and-branch tree with root node (note: Candidate set interval is [min, max))
    CandidateSet root_candidates(*this, from, to + 1);
    if (root_candidates.IsFeasible(validator) &&
        root_candidates.df_max >= worst_d)
      branch_and_bound.emplace(root_candidates.df_max, root_candidates);
  }

  // set max_iterations only if non-exhaustive and predictive solving is enabled.
  // otherwise use predefined value.
  if (!exhaustive && predict)
    max_iterations = tick_iterations;

  while (!branch_and_bound.empty()) {
    /* now loop over the tree, branching each found candidate set, adding the branch if it's feasible.
     * remove all candidate sets with d_max smaller than d_min of the largest integral candidate set
     * always work on the node with largest d_min
     */

    iterations++;

    // break loop if max_iterations or max_tree_size exceeded
    if (iterations > max_iterations || branch_and_bound.size() > max_tree_size)
      break;

    // first clean up tree, removeing all nodes with d_max < worst_d
    branch_and_bound.erase(branch_and_bound.begin(), branch_and_bound.lower_bound(worst_d));

    // we might have cleaned up the whole tree. nothing to do then...
    if (branch_and_bound.empty())
      break;

    /* get node to work on.
     * change node selection strategy if the tree grows too big.
     * this is a mixed depht-first/breadth-first approach, the latter
     * beeing faster, but the first a lot more memory efficient.
     */
    std::multimap<unsigned, CandidateSet>::iterator node;

    if (branch_and_bound.size() > n_points * 4 && iterations % 16 != 0) {
      node = branch_and_bound.upper_bound(branch_and_bound.rbegin()->first / 2);
      if (node == branch_and_bound.end()) --node;
    } else {
      node = --branch_and_bound.end();
    }

    if (node->second.df_min >= worst_d &&
        node->second.IsIntegral(*this, validator)) {
      // node is integral feasible -> a possible solution

      worst_d = node->second.df_min;

      tp1 = node->second.tp1.index_min;
      tp2 = node->second.tp2.index_min;
      tp3 = node->second.tp3.index_min;
      best_d = node->first;

      integral_feasible = true;

    } else {
      // split largest bounding box of node and create child nodes

      const unsigned tp1_diag = node->second.tp1.GetDiagnoal();
      const unsigned tp2_diag = node->second.tp2.GetDiagnoal();
      const unsigned tp3_diag = node->second.tp3.GetDiagnoal();

      const unsigned max_diag = std::max({tp1_diag, tp2_diag, tp3_diag});

      if (tp1_diag == max_diag && node->second.tp1.GetSize() != 1) {
        // split tp1 range
        const unsigned split = (node->second.tp1.index_min + node->second.tp1.index_max) / 2;

        if (split <= node->second.tp2.index_max) {
          CheckAddCandidate(worst_d, validator,
                            {TurnPointRange(*this, node->second.tp1.index_min, split),
                             node->second.tp2, node->second.tp3});

          CheckAddCandidate(worst_d, validator,
                            {TurnPointRange(*this, split, node->second.tp1.index_max),
                             node->second.tp2, node->second.tp3});
        }
      } else if (tp2_diag == max_diag && node->second.tp2.GetSize() != 1) {
        // split tp2 range
        const unsigned split = (node->second.tp2.index_min + node->second.tp2.index_max) / 2;

        if (split <= node->second.tp3.index_max && split >= node->second.tp1.index_min) {
          CheckAddCandidate(worst_d, validator,
                            {node->second.tp1,
                             TurnPointRange(*this, node->second.tp2.index_min, split),
                             node->second.tp3});

          CheckAddCandidate(worst_d, validator,
                            {node->second.tp1,
                             TurnPointRange(*this, split, node->second.tp2.index_max),
                             node->second.tp3});
        }
      } else if (node->second.tp3.GetSize() != 1) {
        // split tp3 range
        const unsigned split = (node->second.tp3.index_min + node->second.tp3.index_max) / 2;

        if (split >= node->second.tp2.index_min) {
          CheckAddCandidate(worst_d, validator,
                            {node->second.tp1, node->second.tp2,
                             TurnPointRange(*this, node->second.tp3.index_min, split)});

          CheckAddCandidate(worst_d, validator,
                            {node->second.tp1, node->second.tp2,
                             TurnPointRange(*this, split, node->second.tp3.index_max)});
        }
      }
    }

    // remove current node
    branch_and_bound.erase(node);
  }


  if (branch_and_bound.empty())
    running = false;

  if (integral_feasible) {
    if (tp1 > tp2) std::swap(tp1, tp2);
    if (tp2 > tp3) std::swap(tp2, tp3);
    if (tp1 > tp2) std::swap(tp1, tp2);

    return {tp1, tp2, tp3, best_d};
  } else {
    return {0, 0, 0, 0};
  }
}

ContestResult
TriangleContest::CalculateResult() const noexcept
{
  ContestResult result;
  result.time = (is_complete && is_closed)
    ? solution[4].DeltaTime(solution[0])
    : 0.;
  result.distance = (is_complete && is_closed)
    ? CalcLegDistance(solution, 0) + CalcLegDistance(solution, 1) + CalcLegDistance(solution, 2)
    : 0.;
  result.score = ApplyHandicap(result.distance * 0.001);
  return result;
}

gcc_pure
static bool
IsInRange(const SearchPoint &a, const SearchPoint &b,
          unsigned half_max_range_sq, double max_distance) noexcept
{
  /* optimisation: if the flat distance is much smaller than the
     maximum range, we don't need to call the method
     GeoPoint::Distance() which is very expensive */
  return a.GetFlatLocation().DistanceSquared(b.GetFlatLocation()) <= half_max_range_sq ||
    a.GetLocation().DistanceS(b.GetLocation()) <= max_distance;
}

bool
TriangleContest::FindClosingPairs(unsigned old_size) noexcept
{
  if (predict) {
    return closing_pairs.Insert(ClosingPair(0, n_points-1));
  }

  struct TracePointNode {
    const TracePoint *point;
    unsigned index;
  };

  struct TracePointNodeAccessor {
    gcc_pure
    int GetX(const TracePointNode &node) const noexcept {
      return node.point->GetFlatLocation().x;
    }

    gcc_pure
    int GetY(const TracePointNode &node) const noexcept {
      return node.point->GetFlatLocation().y;
    }
  };

  QuadTree<TracePointNode, TracePointNodeAccessor> search_point_tree;

  for (unsigned i = old_size; i < n_points; ++i) {
    TracePointNode node;
    node.point = &GetPoint(i);
    node.index = i;

    search_point_tree.insert(node);
  }

  search_point_tree.Optimise();

  bool new_pair = false;

  for (unsigned i = old_size; i < n_points; ++i) {
    TracePointNode point;
    point.point = &GetPoint(i);
    point.index = i;

    const SearchPoint start = *point.point;
    const unsigned max_range = trace_master.ProjectRange(start.GetLocation(), max_distance);
    const unsigned half_max_range_sq = max_range * max_range / 2;

    const int min_altitude = GetMinimumFinishAltitude(*point.point);
    const int max_altitude = GetMaximumStartAltitude(*point.point);

    unsigned last = 0, first = i;

    const auto visitor = [i, start,
                          half_max_range_sq,
                          min_altitude, max_altitude,
                          &first, &last]
      (const TracePointNode &node) {
      const auto &dest = *node.point;

      if (node.index + 2 < i &&
          dest.GetIntegerAltitude() <= max_altitude &&
          IsInRange(start, dest, half_max_range_sq, max_distance)) {
        // point i is last point
        first = std::min(node.index, first);
        last = i;
      } else if (node.index > i + 2 &&
                 dest.GetIntegerAltitude() >= min_altitude &&
                 IsInRange(start, dest, half_max_range_sq, max_distance)) {
        // point i is first point
        first = i;
        last = std::max(node.index, last);
      }
    };

    search_point_tree.VisitWithinRange(point, max_range, visitor);

    if (last != 0 && closing_pairs.Insert(ClosingPair(first, last)))
      new_pair = true;
  }

  return new_pair;
}

bool
TriangleContest::UpdateScore() noexcept
{
  return false;
}

void
TriangleContest::CopySolution(ContestTraceVector &result) const noexcept
{
  result = solution;
  assert(result.size() == 5);
}
