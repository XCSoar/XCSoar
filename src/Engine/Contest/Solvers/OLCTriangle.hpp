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

#ifndef OLC_TRIANGLE_HPP
#define OLC_TRIANGLE_HPP

#include "AbstractContest.hpp"
#include "TraceManager.hpp"
#include "Trace/Point.hpp"

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

    bool insert(const ClosingPair &p) {
      auto found = findRange(p);
      if (found.first == 0 && found.second == 0) {
        closing_pairs[p.first] = p.second;
        removeRange(p.first + 1, p.second);
        return true;
      } else {
        return false;
      }
    }
    
    ClosingPair findRange(const ClosingPair &p) const {
      for (auto it = closing_pairs.begin(); it != closing_pairs.end(); ++it)
        if (it->first <= p.first && it->second >= p.second)
          return ClosingPair(it->first, it->second);

      return ClosingPair(0, 0);
    }

    void removeRange(unsigned first, unsigned last) {
      auto it = closing_pairs.begin();
      while (it != closing_pairs.end()) {
        if (it->first > first && it->second < last)
          it = closing_pairs.erase(it);
        else
          it++;
      }
    }

    void clear() {
      closing_pairs.clear();
    }
  };

  ClosingPairs closing_pairs;


  /**
   * kd-tree node of a trace point. Used for nearest search to find
   * closed track loops
   */
  struct TracePointNode {
    typedef int value_type;

    const TracePoint *point;
    unsigned index;

    value_type operator[](unsigned n) const {
      return n ? point->GetFlatLocation().longitude : point->GetFlatLocation().latitude;
    }

    unsigned distance(const TracePointNode &node) const {
      const int lon = point->GetFlatLocation().longitude;
      const int lat = point->GetFlatLocation().latitude;

      return std::max(fabs(lon), fabs(lat));
    }
  };

  /**
   * A bounding box around a range of trace points.
   */
  struct TurnPointRange {
    unsigned index_min, index_max; // [index_min, index_max)
    int lon_min, lon_max,
        lat_min, lat_max;

    TurnPointRange() :
      index_min(0), index_max(0),
      lon_min(0), lon_max(0),
      lat_min(0), lat_max(0) {}

    TurnPointRange(OLCTriangle *parent, unsigned min, unsigned max) {
      update(parent, min, max);
    }

    TurnPointRange(const TurnPointRange &other) {
      index_min = other.index_min;
      index_max = other.index_max;
      lon_min = other.lon_min;
      lon_max = other.lon_max;
      lat_min = other.lat_min;
      lat_max = other.lat_max;
    }

    bool operator==(TurnPointRange other) const {
      return (index_min == other.index_min && index_max == other.index_max);
    }

    // returns the manhatten diagonal of the bounding box
    unsigned diagonal() const {
      unsigned width = abs(lon_max - lon_min);
      unsigned height = abs(lat_max - lat_min);
      return width + height;
    }

    // returns the number of points in this range
    unsigned size() const {
      return index_max - index_min;
    }

    // updates the bounding box by a given point range
    void update(OLCTriangle *parent, unsigned &_min, unsigned &_max) {
      lon_min = parent->GetPoint(_min).GetFlatLocation().longitude;
      lon_max = parent->GetPoint(_min).GetFlatLocation().longitude;
      lat_min = parent->GetPoint(_min).GetFlatLocation().latitude;
      lat_max = parent->GetPoint(_min).GetFlatLocation().latitude;

      for (unsigned i = _min + 1; i < _max; ++i) {
        lon_min = std::min(lon_min, parent->GetPoint(i).GetFlatLocation().longitude);
        lon_max = std::max(lon_max, parent->GetPoint(i).GetFlatLocation().longitude);
        lat_min = std::min(lat_min, parent->GetPoint(i).GetFlatLocation().latitude);
        lat_max = std::max(lat_max, parent->GetPoint(i).GetFlatLocation().latitude);
      }

      index_min = _min;
      index_max = _max;
    }

    // calculate the minimal distance estimate between two TurnPointRanges
    unsigned min_dist(const TurnPointRange &tp) const {
      const unsigned d_lon = std::max(tp.lon_min - lon_max, lon_min - tp.lon_max) < 0 ?
                       0 :
                       std::min(abs(tp.lon_min - lon_max), abs(tp.lon_max - lon_min));

      const unsigned d_lat = std::max(tp.lat_min - lat_max, lat_min - tp.lat_max) < 0 ?
                       0 :
                       std::min(abs(tp.lat_min - lat_max), abs(tp.lat_max - lat_min));

      return sqrt(d_lon*d_lon + d_lat*d_lat);
    }

    // calculate maximal distance estimate between two TurnPointRanges
    unsigned max_dist(const TurnPointRange &tp) const {
      const unsigned d_lon = std::max(lon_max - tp.lon_min, tp.lon_max - lon_min);
      const unsigned d_lat = std::max(lat_max - tp.lat_min, tp.lat_max - lat_min);

      return sqrt(d_lon*d_lon + d_lat*d_lat);
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

    CandidateSet(OLCTriangle *parent, unsigned first, unsigned last) {
      tp1.update(parent, first, last);
      tp2.update(parent, first, last);
      tp3.update(parent, first, last);

      updateDistances();
    }
    
    CandidateSet(TurnPointRange _tp1, TurnPointRange _tp2, TurnPointRange _tp3) {
      tp1 = _tp1;
      tp2 = _tp2;
      tp3 = _tp3;

      updateDistances();
    }

    void updateDistances() {  
      const unsigned df_12_min = tp1.min_dist(tp2),
                     df_23_min = tp2.min_dist(tp3),
                     df_31_min = tp3.min_dist(tp1);
      
      const unsigned df_12_max = tp1.max_dist(tp2),
                     df_23_max = tp2.max_dist(tp3),
                     df_31_max = tp3.max_dist(tp1);

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
    bool isFeasible(const bool fai, const unsigned large_triangle_check) const {
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
    bool integral(OLCTriangle *parent, const bool fai, const unsigned large_triangle_check) const {
      if (!(tp1.size() == 1 && tp2.size() == 1 && tp3.size() == 1))
        return false;

      if (!fai) return true;

      assert(df_min == df_max);

      // fast checks, as in isFeasible

      // shortest >= 28.2% * dist_total
      if (shortest_max * 39 >= df_max * 11)
        return true;

      // longest >= 45.8% * dist_total
      if (longest_max * 24 > df_max * 11)
        return false;

      // small triangle and shortest < 27.5% dist_total
      if (df_max < large_triangle_check && shortest_max * 40 < df_max * 11)
        return false;

      // detailed checks
      auto geo_tp1 = parent->GetPoint(tp1.index_min).GetLocation();
      auto geo_tp2 = parent->GetPoint(tp2.index_min).GetLocation();
      auto geo_tp3 = parent->GetPoint(tp3.index_min).GetLocation();

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

  void UpdateTrace(bool force);
  void ResetBranchAndBound();

public:
  /* virtual methods from AbstractContest */
  virtual void Reset() override;
  virtual SolverResult Solve(bool exhaustive) override;

  void SetMaxIterations(unsigned _max_iterations) {
    max_iterations = _max_iterations;
  };

  void SetMaxTreeSize(unsigned _max_tree_size) {
    max_tree_size = _max_tree_size;
  };

protected:
  /* virtual methods from AbstractContest */
  virtual bool UpdateScore() override;
  virtual void CopySolution(ContestTraceVector &vec) const override;
  virtual ContestResult CalculateResult() const override;
};

#endif
