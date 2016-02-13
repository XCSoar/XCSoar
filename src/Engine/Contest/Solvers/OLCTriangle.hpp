/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef OLC_TRIANGLE_HPP
#define OLC_TRIANGLE_HPP

#include "AbstractContest.hpp"
#include "TraceManager.hpp"
#include "Trace/Point.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"

#include <map>

/**
 * Specialisation of AbstractContest for OLC Triangle (triangle) rules
 */
class OLCTriangle : public AbstractContest, public TraceManager {
protected:
  const bool is_fai;

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
  bool is_closed;

  /**
   * True if a valid triangle is found
   */
  bool is_complete;

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
  unsigned max_iterations,
           max_tree_size;

  typedef std::pair<unsigned, unsigned> ClosingPair;

  struct ClosingPairs {
    std::map<unsigned, unsigned> closing_pairs;

    bool Insert(const ClosingPair &p) {
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

    gcc_pure
    ClosingPair FindRange(const ClosingPair &p) const {
      for (const auto &i : closing_pairs) {
        if (i.first > p.first)
          break;

        if (i.second >= p.second)
          return i;
      }

      return ClosingPair(0, 0);
    }

    void RemoveRange(std::map<unsigned, unsigned>::iterator it,
                     unsigned last) {
      const auto end = closing_pairs.end();
      while (it != end) {
        if (it->second < last)
          it = closing_pairs.erase(it);
        else
          it++;
      }
    }

    void Clear() {
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

    TurnPointRange()
      :index_min(0), index_max(0),
       bounding_box(FlatGeoPoint(0, 0)) {}

    TurnPointRange(const OLCTriangle &parent, unsigned min, unsigned max) {
      Update(parent, min, max);
    }

    bool operator==(TurnPointRange other) const {
      return (index_min == other.index_min && index_max == other.index_max);
    }

    // returns the manhatten diagonal of the bounding box
    gcc_pure
    unsigned GetDiagnoal() const {
      return bounding_box.GetWidth() + bounding_box.GetHeight();
    }

    // returns the number of points in this range
    unsigned GetSize() const {
      return index_max - index_min;
    }

    // updates the bounding box by a given point range
    void Update(const OLCTriangle &parent, unsigned _min, unsigned _max) {
      bounding_box = FlatBoundingBox(parent.GetPoint(_min).GetFlatLocation());

      for (unsigned i = _min + 1; i < _max; ++i)
        bounding_box.Expand(parent.GetPoint(i).GetFlatLocation());

      index_min = _min;
      index_max = _max;
    }

    // calculate the minimal distance estimate between two TurnPointRanges
    gcc_pure
    unsigned GetMinDistance(const TurnPointRange &tp) const {
      return bounding_box.Distance(tp.bounding_box);
    }

    // calculate maximal distance estimate between two TurnPointRanges
    gcc_pure
    unsigned GetMaxDistance(const TurnPointRange &tp) const {
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

    CandidateSet() :
      df_min(0), df_max(0),
      shortest_max(0), longest_min(0), longest_max(0) {}

    CandidateSet(const OLCTriangle &parent, unsigned first, unsigned last)
      :tp1(parent, first, last), tp2(tp1), tp3(tp1) {
      UpdateDistances();
    }

    CandidateSet(TurnPointRange _tp1, TurnPointRange _tp2, TurnPointRange _tp3)
      :tp1(_tp1), tp2(_tp2), tp3(_tp3) {
      UpdateDistances();
    }

    void UpdateDistances() {
      const unsigned df_12_min = tp1.GetMinDistance(tp2),
                     df_23_min = tp2.GetMinDistance(tp3),
                     df_31_min = tp3.GetMinDistance(tp1);

      const unsigned df_12_max = tp1.GetMaxDistance(tp2),
                     df_23_max = tp2.GetMaxDistance(tp3),
                     df_31_max = tp3.GetMaxDistance(tp1);

      shortest_max = std::min({df_12_max, df_23_max, df_31_max});
      longest_min = std::max({df_12_min, df_23_min, df_31_min});
      longest_max = std::max({df_12_max, df_23_max, df_31_max});

      df_min = std::max(df_12_min + df_23_min + df_31_min,
                        longest_min * 2);
      df_max = std::min(df_12_max + df_23_max + df_31_max,
                        shortest_max * 4);
    }

    bool operator==(CandidateSet other) const {
      return (tp1 == other.tp1 && tp2 == other.tp2 && tp3 == other.tp3);
    }

    /* Calculates if this candidate set is feasible
     * (i.e. it might contain a feasible triangle).
     * Use relaxed checks to ensure distance errors due to the flat projection
     * or integer rounding don't invalidate close positives.
     */
    gcc_pure
    bool IsFeasible(const bool fai, const unsigned large_triangle_check) const {
      // always feasible if no fai constraints
      if (!fai) return true;

      // shortest leg min 28% (here: 27.5%) for small triangle,
      // min 25% (here: 24.3%) for large triangle
      if ((df_max > large_triangle_check && shortest_max * 37 < df_min * 9) ||
          (df_max <= large_triangle_check && shortest_max * 40 < df_min * 11)) {
        return false;
      }

      // longest leg max 45% (here: 47%)
      if (longest_min * 19 > df_max * 9) {
        return false;
      }

      return true;
    }

    /* Check if the candidate set is a real fai triangle. Use fast checks on projected
     * distances for certain checks, otherwise real distances for marginal fai triangles.
     */
    gcc_pure
    bool IsIntegral(OLCTriangle &parent, const bool fai,
                    const unsigned large_triangle_check) const {
      if (!(tp1.GetSize() == 1 && tp2.GetSize() == 1 && tp3.GetSize() == 1))
        return false;

      if (!fai) return true;

      // Solution is integral, calculate rough distance for fast checks
      const unsigned df_total = tp1.GetMaxDistance(tp2) +
                                tp2.GetMaxDistance(tp3) +
                                tp3.GetMaxDistance(tp1);

      // fast checks, as in IsFeasible

      // shortest >= 28.2% * df_total
      if (shortest_max * 39 >= df_total * 11)
        return true;

      // longest >= 45.8% * df_total
      if (longest_max * 24 > df_total * 11)
        return false;

      // small triangle and shortest < 27.5% df_total
      if (df_total < large_triangle_check && shortest_max * 40 < df_total * 11)
        return false;

      // detailed checks
      auto geo_tp1 = parent.GetPoint(tp1.index_min).GetLocation();
      auto geo_tp2 = parent.GetPoint(tp2.index_min).GetLocation();
      auto geo_tp3 = parent.GetPoint(tp3.index_min).GetLocation();

      const unsigned d_12 = unsigned(geo_tp1.Distance(geo_tp2));
      const unsigned d_23 = unsigned(geo_tp2.Distance(geo_tp3));
      const unsigned d_31 = unsigned(geo_tp3.Distance(geo_tp1));

      const unsigned d_total = d_12 + d_23 + d_31;

      // real check of 28% rule
      const unsigned shortest = std::min({d_12, d_23, d_31});
      if (shortest * 25 >= d_total * 7)
        return true;

      // real check of 45% rule
      const unsigned longest = std::max({d_12, d_23, d_31});
      if (longest * 20 > d_total * 9)
        return false;

      // real check of 25% for dist_total >= 500km rule
      if (d_total >= 500000 && shortest * 4 >= d_total)
        return true;

      return false;
    }
  };

  std::multimap<unsigned, CandidateSet> branch_and_bound;

public:
  OLCTriangle(const Trace &_trace,
              bool is_fai,
              bool predict,
              const unsigned finish_alt_diff = 1000);

  void SetIncremental(bool _incremental) {
    incremental = _incremental;
  }

protected:
  bool FindClosingPairs(unsigned old_size);
  void SolveTriangle(bool exhaustive);

  std::tuple<unsigned, unsigned, unsigned, unsigned>
  RunBranchAndBound(unsigned from, unsigned to, unsigned best_d, bool exhaustive);

  void UpdateTrace(bool force) override;
  void ResetBranchAndBound();

public:
  void SetMaxIterations(unsigned _max_iterations) {
    max_iterations = _max_iterations;
  };

  void SetMaxTreeSize(unsigned _max_tree_size) {
    max_tree_size = _max_tree_size;
  };

  /* virtual methods from AbstractContest */
  void Reset() override;
  SolverResult Solve(bool exhaustive) override;

protected:
  /* virtual methods from AbstractContest */
  bool UpdateScore() override;
  void CopySolution(ContestTraceVector &vec) const override;
  ContestResult CalculateResult() const override;
};

#endif
