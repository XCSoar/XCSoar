/*
Copyright_License {

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

#ifndef NAV_DIJKSTRA_HPP
#define NAV_DIJKSTRA_HPP

#include "Dijkstra.hpp"
#include "ScanTaskPoint.hpp"
#include "SolverResult.hpp"
#include "Compiler.h"

#include <unordered_map>
#include <assert.h>

/**
 * Abstract class for A* /Dijkstra searches of nav points, managing
 * edges in multiple stages (corresponding to turn points).
 *
 * Expected running time, see http://www.avglab.com/andrew/pub/neci-tr-96-062.ps
 *
 * NavDijkstra<SearchPoint>
 */
class NavDijkstra {
protected:
  static constexpr unsigned MAX_STAGES = 32;

  struct DijkstraMap {
    struct Hash {
      std::size_t operator()(ScanTaskPoint p) const {
        return p.Key();
      }
    };

    struct Equal {
      std::size_t operator()(ScanTaskPoint a, ScanTaskPoint b) const {
        return a.Key() == b.Key();
      }
    };

    template<typename Value>
    struct Bind : public std::unordered_map<ScanTaskPoint, Value,
                                            Hash, Equal> {
    };
  };

  typedef ::Dijkstra<ScanTaskPoint, DijkstraMap> Dijkstra;

  Dijkstra dijkstra;

  /** Number of stages in search */
  unsigned num_stages;

  /**
   * An array containing the point index for each of the solution's stages.
   */
  unsigned solution[MAX_STAGES];

protected:
  /**
   * Constructor
   *
   * @param _num_stages Number of stages in search
   *
   * @return Initialised object
   */
  NavDijkstra(const unsigned _num_stages)
  {
    SetStageCount(_num_stages);
  }

  NavDijkstra(const NavDijkstra &) = delete;

protected:
  /**
   * Set the number of stages to search for, and clear the solution
   * array
   */
  void SetStageCount(const unsigned _num_stages) {
    assert(_num_stages <= MAX_STAGES);
    num_stages =_num_stages;
  }

  /**
   * Determine whether a finished path is valid
   *
   * @param sp Point to check
   *
   * @return True if this terminal point completes a valid solution
   */
  bool IsFinishSatisfied(gcc_unused const ScanTaskPoint sp) const {
    return true;
  }

  /**
   * Add edges from an origin node
   *
   * @param curNode Origin node to add edges from
   */
  virtual void AddEdges(const ScanTaskPoint curNode) = 0;

  gcc_pure
  bool IsFinal(unsigned stage_number) const {
    assert(stage_number < num_stages);

    return stage_number + 1 == num_stages;
  }

  /**
   * Determine whether a point is terminal (no further edges)
   *
   * @param sp Point to test
   *
   * @return True if point is terminal
   */
  gcc_pure
  bool IsFinal(const ScanTaskPoint sp) const {
    assert(num_stages <= MAX_STAGES);

    return IsFinal(sp.GetStageNumber());
  }

  bool Link(const ScanTaskPoint node, const ScanTaskPoint parent,
            unsigned value) {
    return dijkstra.Link(node, parent, value);
  }

  void LinkStart(const ScanTaskPoint node, unsigned value=0) {
    Link(node, node, value);
  }

  /**
   * Iterate search algorithm
   *
   * @param dijkstra Dijkstra structure to iterate
   * @param max_steps Maximum number of steps to update
   *
   * @return True if algorithm returns a terminal path or no path found
   */
  SolverResult DistanceGeneral(unsigned max_steps = 0 - 1) {
    while (!dijkstra.IsEmpty()) {
      const ScanTaskPoint destination = dijkstra.Pop();

      if (IsFinal(destination)) {
        FindSolution(destination);
        if (IsFinishSatisfied(destination))
          return SolverResult::VALID;
      } else {
        AddEdges(destination);
        if (dijkstra.IsEmpty())
          /* error, no way to reach final */
          return SolverResult::FAILED;
      }

      if (max_steps)
        --max_steps;
      else
        /* reached limit */
        return dijkstra.IsEmpty()
          ? SolverResult::FAILED
          : SolverResult::INCOMPLETE;
    }

    /* no path found */
    return SolverResult::FAILED;
  }

  /**
   * Search the chain for the ScanTaskPoint at the specified stage.
   */
  gcc_pure
  ScanTaskPoint FindStage(ScanTaskPoint p, unsigned stage_number) const {
    assert(stage_number <= p.GetStageNumber());

    while (p.GetStageNumber() > stage_number) {
#ifndef NDEBUG
      const ScanTaskPoint o(p);
#endif
      p = dijkstra.GetPredecessor(p);
      assert(p.GetStageNumber() + 1 == o.GetStageNumber());
    }

    return p;
  }

  /**
   * Find the first ScanTaskPoint in the chain.
   */
  gcc_pure
  ScanTaskPoint FindStart(ScanTaskPoint p) const {
    return FindStage(p, 0);
  }

  /**
   * Determine optimal solution by backtracing the Dijkstra tree
   *
   * @param destination Terminal point to query
   */
  void FindSolution(const ScanTaskPoint destination) {
    ScanTaskPoint p(destination);
    unsigned last_stage_number;

    do {
      last_stage_number = p.GetStageNumber();
      solution[p.GetStageNumber()] = p.GetPointIndex();
      p = dijkstra.GetPredecessor(p);
    } while (p.GetStageNumber() != last_stage_number);
  }
};

#endif
