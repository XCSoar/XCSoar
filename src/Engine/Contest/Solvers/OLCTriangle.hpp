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

#include "ContestDijkstra.hpp"

#include <unordered_set>

/**
 * Specialisation of OLC Dijkstra for OLC Triangle (triangle) rules
 */
class OLCTriangle : public ContestDijkstra {
protected:
  const bool is_fai;

private:
  /**
   * Assume the the pilot will reach the start point?  This is useful
   * for continuous (live) calculation, to give the pilot an estimate
   * what his score will be.  Without this, this class is not useful
   * for continuous calculation, as you will only see the result after
   * the pilot has successfully finished the task.
   */
  const bool predict;

  bool is_closed;
  bool is_complete;
  unsigned first_tp;
  unsigned closing; ///< Stores the last found trace-closing start point

  struct ClosingPair {
    unsigned first;
    unsigned last;

    ClosingPair(unsigned _first, unsigned _last)
      :first(_first), last(_last) {}
  };

  struct Hash {
    size_t operator()(ClosingPair p) const {
      return size_t((p.first << 16) | (p.last & 0xffff));
    }
  };

  struct EqualClosing {
    bool operator()(ClosingPair a, ClosingPair b) const {
      return (a.first >= b.first && a.last <= b.last);
    }
  };

  struct EqualNonClosing {
    bool operator()(ClosingPair a, ClosingPair b) const {
      return (a.first <= b.first && a.last >= b.last);
    }
  };

  std::unordered_set<ClosingPair, Hash, EqualClosing> closing_pairs;
  std::unordered_set<ClosingPair, Hash, EqualNonClosing> non_closing_pairs;

protected:
  unsigned best_d;

public:
  OLCTriangle(const Trace &_trace, bool is_fai, bool predict);

protected:
  gcc_pure
  bool IsPathClosed() const;

  /**
   * This method searches for a pair of points which close the path between
   * tp3 (last_index) and tp1 (first_index), accounting for the fai rules of
   * max distance and max height difference.
   *
   * It's essentially brute-force, but using the last found closing point as
   * start value and searching alternating (+ and -) around this point yields
   * a acceptable speed.
   */
  ClosingPair ClosingPoint(unsigned first_index, unsigned last_index,
                           const int min_altitude, unsigned max_range);

  /* adds tp2 */
  void AddTurn1Edges(const ScanTaskPoint origin);

  /* adds tp3 if triangle rules are satisfied */
  void AddTurn2Edges(const ScanTaskPoint origin);

  /* adds start and final (two stages) if they satisfy the closure rules */
  void AddFinishEdges(const ScanTaskPoint origin);

public:
  /* virtual methods from AbstractContest */
  virtual void Reset();

protected:
  /* virtual methods from AbstractContest */
  virtual bool UpdateScore() override;
  virtual void CopySolution(ContestTraceVector &vec) const override;

  /* virtual methods from NavDijkstra */
  virtual void AddStartEdges() override;
  virtual void AddEdges(ScanTaskPoint curNode) override;

  /* virtual methods from ContestDijkstra */
  virtual void StartSearch() override;
  virtual ContestResult CalculateResult(const ContestTraceVector &solution) const override;
};

#endif
