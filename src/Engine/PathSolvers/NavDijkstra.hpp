// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Dijkstra.hpp"
#include "ScanTaskPoint.hpp"
#include "SolverResult.hpp"

#include <unordered_map>
#include <cassert>

/**
 * Abstract class for A* /Dijkstra searches of nav points, managing
 * edges in multiple stages (corresponding to turn points).
 *
 * Expected running time, see http://www.avglab.com/andrew/pub/neci-tr-96-062.ps
 *
 * NavDijkstra<SearchPoint>
 */
template<typename ValueType=unsigned>
class NavDijkstra {
protected:
  static constexpr unsigned MAX_STAGES = 32;

  struct DijkstraMap {
    struct Hash {
      constexpr std::size_t operator()(ScanTaskPoint p) const noexcept {
        return p.Key();
      }
    };

    struct Equal {
      constexpr bool operator()(ScanTaskPoint a,
                                ScanTaskPoint b) const noexcept {
        return a.Key() == b.Key();
      }
    };

    template<typename Value>
    struct Bind : public std::unordered_map<ScanTaskPoint, Value,
                                            Hash, Equal> {
    };
  };

  using Dijkstra = ::Dijkstra<ScanTaskPoint, DijkstraMap, ValueType>;
  using value_type = typename Dijkstra::value_type;

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
  NavDijkstra(const unsigned _num_stages) noexcept
  {
    SetStageCount(_num_stages);
  }

  NavDijkstra(const NavDijkstra &) = delete;

protected:
  /**
   * Set the number of stages to search for, and clear the solution
   * array
   */
  void SetStageCount(const unsigned _num_stages) noexcept {
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
  bool IsFinishSatisfied([[maybe_unused]] const ScanTaskPoint sp) const noexcept {
    return true;
  }

  /**
   * Add edges from an origin node
   *
   * @param curNode Origin node to add edges from
   */
  virtual void AddEdges(const ScanTaskPoint curNode) noexcept = 0;

  [[gnu::pure]]
  bool IsFinal(unsigned stage_number) const noexcept {
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
  [[gnu::pure]]
  bool IsFinal(const ScanTaskPoint sp) const noexcept {
    assert(num_stages <= MAX_STAGES);

    return IsFinal(sp.GetStageNumber());
  }

  bool Link(const ScanTaskPoint node, const ScanTaskPoint parent,
            value_type value) noexcept {
    return dijkstra.Link(node, parent, value);
  }

  void LinkStart(const ScanTaskPoint node, value_type value={}) noexcept {
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
  SolverResult DistanceGeneral(unsigned max_steps = 0 - 1) noexcept {
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
  [[gnu::pure]]
  ScanTaskPoint FindStage(ScanTaskPoint p,
                          unsigned stage_number) const noexcept {
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
  [[gnu::pure]]
  ScanTaskPoint FindStart(ScanTaskPoint p) const noexcept {
    return FindStage(p, 0);
  }

  /**
   * Determine optimal solution by backtracing the Dijkstra tree
   *
   * @param destination Terminal point to query
   */
  void FindSolution(const ScanTaskPoint destination) noexcept {
    ScanTaskPoint p(destination);
    unsigned last_stage_number;

    do {
      last_stage_number = p.GetStageNumber();
      solution[p.GetStageNumber()] = p.GetPointIndex();
      p = dijkstra.GetPredecessor(p);
    } while (p.GetStageNumber() != last_stage_number);
  }
};
