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

#ifndef TRIANGLE_CONTEST_HPP
#define TRIANGLE_CONTEST_HPP

#include "AbstractContest.hpp"
#include "OLCTriangleRules.hpp"
#include "TraceManager.hpp"
#include "Trace/Point.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"

#include <map>

/**
 * Specialisation of AbstractContest for OLC Triangle (triangle) rules
 */
class TriangleContest : public AbstractContest, public TraceManager {
protected:
  /**
   * The last solution.  Use only if Solve() has returned VALID.
   */
  ContestTraceVector solution;

  /* Contains the best flat distance found so far */
  unsigned best_d;

private:
  /**
   * Assume the the pilot will reach the start point?  This is useful
   * for continuous (live) calculation, to give the pilot an estimate
   * what his score will be.  Without this, this class is not useful
   * for continuous calculation, as you will only see the result after
   * the pilot has successfully finished the task.
   */
  const bool predict;

  /**
   * Do an incremental analysis, attempting to improve the result in
   * each iteration?  If set, then the new triangle is considered to be
   * larger than the previous one.
   * This triggers a new closing-point search with the new points when
   * the solver is called.
   */
  bool incremental;

  /**
   * True if at least one closed track loop is found
   */
  bool is_closed = false;

  /**
   * True if a valid triangle is found
   */
  bool is_complete = false;

  /**
   * True if the branch and bound algorithm is running
   */
  bool running;

  /**
   * Number of iterations per tick (only for non-exhaustive,
   * predictive runs)
   */
  unsigned tick_iterations;

  /**
   * Hard limits for number of iterations and tree size.
   */
  unsigned max_iterations = 1e6,
           max_tree_size = 5e5;

  typedef std::pair<unsigned, unsigned> ClosingPair;

  struct ClosingPairs {
    std::map<unsigned, unsigned> closing_pairs;

    bool Insert(const ClosingPair &p) noexcept {
      auto found = FindRange(p);
      if (found.first == 0 && found.second == 0) {
        const auto result = closing_pairs.insert(p);
        if (!result.second)
          result.first->second = p.second;
        RemoveRange(std::next(result.first), p.second);
        return true;
      } else {
        return false;
      }
    }

    [[gnu::pure]]
    ClosingPair FindRange(const ClosingPair &p) const noexcept {
      for (const auto &i : closing_pairs) {
        if (i.first > p.first)
          break;

        if (i.second >= p.second)
          return i;
      }

      return ClosingPair(0, 0);
    }

    void RemoveRange(std::map<unsigned, unsigned>::iterator it,
                     unsigned last) noexcept {
      const auto end = closing_pairs.end();
      while (it != end) {
        if (it->second < last)
          it = closing_pairs.erase(it);
        else
          it++;
      }
    }

    void Clear() noexcept {
      closing_pairs.clear();
    }
  };

  ClosingPairs closing_pairs;

  /**
   * A bounding box around a range of trace points.
   */
  struct TurnPointRange {
    unsigned index_min, index_max; // [index_min, index_max)

    FlatBoundingBox bounding_box;

    TurnPointRange(const TriangleContest &parent,
                   const unsigned min, const unsigned max) noexcept
      :index_min(min), index_max(max),
       bounding_box(FlatBoundingBox(parent.GetPoint(min).GetFlatLocation()))
    {
      for (unsigned i = min + 1; i < max; ++i)
        bounding_box.Expand(parent.GetPoint(i).GetFlatLocation());
    }

    bool operator==(TurnPointRange other) const noexcept {
      return (index_min == other.index_min && index_max == other.index_max);
    }

    // returns the manhatten diagonal of the bounding box
    [[gnu::pure]]
    unsigned GetDiagnoal() const noexcept {
      return bounding_box.GetWidth() + bounding_box.GetHeight();
    }

    // returns the number of points in this range
    unsigned GetSize() const noexcept {
      return index_max - index_min;
    }

    // calculate the minimal distance estimate between two TurnPointRanges
    [[gnu::pure]]
    unsigned GetMinDistance(const TurnPointRange &tp) const noexcept {
      return bounding_box.Distance(tp.bounding_box);
    }

    // calculate maximal distance estimate between two TurnPointRanges
    [[gnu::pure]]
    unsigned GetMaxDistance(const TurnPointRange &tp) const noexcept {
      const unsigned d_lon = std::max(bounding_box.GetRight() - tp.bounding_box.GetLeft(),
                                      tp.bounding_box.GetRight() - bounding_box.GetLeft());
      const unsigned d_lat = std::max(bounding_box.GetTop() - tp.bounding_box.GetBottom(),
                                      tp.bounding_box.GetTop() - bounding_box.GetBottom());

      return hypot(d_lon, d_lat);
    }
  };

  /**
   * A set of three TurnPointRanges which form a triangle
   */
  struct CandidateSet {
    TurnPointRange tp1, tp2, tp3;
    unsigned df_min, df_max;
    unsigned shortest_max, longest_min, longest_max;

    CandidateSet(TurnPointRange _tp1, TurnPointRange _tp2,
                 TurnPointRange _tp3) noexcept
      :tp1(_tp1), tp2(_tp2), tp3(_tp3)
    {
      const unsigned df_12_min = tp1.GetMinDistance(tp2),
                     df_23_min = tp2.GetMinDistance(tp3),
                     df_31_min = tp3.GetMinDistance(tp1);

      const unsigned df_12_max = tp1.GetMaxDistance(tp2),
                     df_23_max = tp2.GetMaxDistance(tp3),
                     df_31_max = tp3.GetMaxDistance(tp1);

      std::tie(shortest_max, longest_max) = std::minmax({df_12_max, df_23_max, df_31_max});
      longest_min = std::max({df_12_min, df_23_min, df_31_min});

      df_min = std::max(df_12_min + df_23_min + df_31_min,
                        longest_min * 2);
      df_max = std::min(df_12_max + df_23_max + df_31_max,
                        shortest_max * 4);
    }

    explicit CandidateSet(TurnPointRange tp) noexcept
      :CandidateSet(tp, tp, tp) {}

    CandidateSet(const TriangleContest &parent,
                 unsigned first, unsigned last) noexcept
      :CandidateSet(TurnPointRange{parent, first, last}) {}

    bool operator==(CandidateSet other) const noexcept {
      return (tp1 == other.tp1 && tp2 == other.tp2 && tp3 == other.tp3);
    }

    /* Calculates if this candidate set is feasible
     * (i.e. it might contain a feasible triangle).
     * Use relaxed checks to ensure distance errors due to the flat projection
     * or integer rounding don't invalidate close positives.
     */
    [[gnu::pure]]
    bool IsFeasible(const OLCTriangleValidator &validator) const noexcept {
      return validator.IsFeasible(df_min, df_max,
                                  shortest_max, longest_min);
    }

    /* Check if the candidate set is a real fai triangle. Use fast checks on projected
     * distances for certain checks, otherwise real distances for marginal fai triangles.
     */
    [[gnu::pure]]
    bool IsIntegral(TriangleContest &parent,
                    const OLCTriangleValidator &validator) const noexcept {
      if (!(tp1.GetSize() == 1 && tp2.GetSize() == 1 && tp3.GetSize() == 1))
        return false;

      return validator.IsIntegral(tp1, tp2, tp3,
                                  shortest_max, longest_max,
                                  [](const auto &a, const auto &b){
                                    return a.GetMaxDistance(b);
                                  },
                                  [&parent](const auto &tp){
                                    return parent.GetPoint(tp.index_min).GetLocation();
                                  });
    }
  };

  std::multimap<unsigned, CandidateSet> branch_and_bound;

public:
  TriangleContest(const Trace &_trace,
                  bool predict,
                  const unsigned finish_alt_diff = 1000) noexcept;

  void SetIncremental(bool _incremental) noexcept {
    incremental = _incremental;
  }

protected:
  bool FindClosingPairs(unsigned old_size) noexcept;
  void SolveTriangle(bool exhaustive) noexcept;

  std::tuple<unsigned, unsigned, unsigned, unsigned>
  RunBranchAndBound(unsigned from, unsigned to, unsigned best_d,
                    bool exhaustive) noexcept;

  void UpdateTrace(bool force) noexcept override;
  void ResetBranchAndBound() noexcept;

private:
  void CheckAddCandidate(unsigned worst_d,
                         const OLCTriangleValidator &validator,
                         CandidateSet candidate_set) noexcept {
    if (candidate_set.df_max >= worst_d &&
        candidate_set.IsFeasible(validator))
      branch_and_bound.emplace(candidate_set.df_max, candidate_set);
  }

public:
  void SetMaxIterations(unsigned _max_iterations) noexcept {
    max_iterations = _max_iterations;
  };

  void SetMaxTreeSize(unsigned _max_tree_size) noexcept {
    max_tree_size = _max_tree_size;
  };

  /* virtual methods from AbstractContest */
  void Reset() noexcept override;
  SolverResult Solve(bool exhaustive) noexcept override;

protected:
  /* virtual methods from AbstractContest */
  bool UpdateScore() noexcept override;
  void CopySolution(ContestTraceVector &vec) const noexcept override;
  ContestResult CalculateResult() const noexcept override;
};

#endif
